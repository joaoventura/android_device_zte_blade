/*
 * Copyright (C) 2012 Grigori Goronzy <greg@kinoho.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "utils/Log.h"
#include "hardware/fmradio.h"

#define LOG_TAG "libfmradio_si4708"

/* band */
#define BAND_87500_108000_kHz   0
#define BAND_76000_108000_kHz   1
#define BAND_76000_90000_kHz    2

/* channel spacing */
#define CHAN_SPACING_200_kHz   0        /* US */
#define CHAN_SPACING_100_kHz   1        /* Europe, Japan */
#define CHAN_SPACING_50_kHz    2

/* seeking */
#define      SEEKUP             1
#define      SEEKDOWN           0

/* magic no */
#define Si4708_IOC_MAGIC  'k'

/* commands */
#define Si4708_IOC_INIT2NORMAL                      _IO(Si4708_IOC_MAGIC, 1)
#define Si4708_IOC_NORMAL2STANDBY                   _IO(Si4708_IOC_MAGIC, 2)
#define Si4708_IOC_STANDBY2NORMAL                   _IO(Si4708_IOC_MAGIC, 3)
#define Si4708_IOC_BAND_SET                         _IOW(Si4708_IOC_MAGIC, 12, int)
#define Si4708_IOC_CHAN_SPACING_SET                 _IOW(Si4708_IOC_MAGIC, 14, int)
#define Si4708_IOC_CHAN_SELECT                      _IOW(Si4708_IOC_MAGIC, 4, int)
#define Si4708_IOC_CHAN_SEEK                        _IOW(Si4708_IOC_MAGIC, 5, int[2])
#define Si4708_IOC_CHAN_GET                         _IOR(Si4708_IOC_MAGIC, 17, int)
#define Si4708_IOC_CUR_RSSI_GET                     _IOR(Si4708_IOC_MAGIC, 12, rssi_snr_t)
#define Si4708_IOC_VOLUME_GET                       _IOR(Si4708_IOC_MAGIC, 7, int)
#define Si4708_IOC_VOLUME_SET                       _IOW(Si4708_IOC_MAGIC, 8, int)
#define Si4708_IOC_RDS_ENABLE                       _IO(Si4708_IOC_MAGIC, 23)
#define Si4708_IOC_RDS_DISABLE                      _IO(Si4708_IOC_MAGIC, 24)
#define Si4708_IOC_DE_SET                           _IOW(Si4708_IOC_MAGIC,32,uint8_t)  /* Setting DE-emphasis Time Constant. For DE=0,TC=50us(Europe,Japan,Australia) and DE=1,TC=75us(USA) */
#define Si4708_IOC_SET_AUDIOTRACK _IOW(Si4708_IOC_MAGIC, 16, int)

/* state */

struct si4708_session {
    int fd;
    bool radioInitialised;
    bool radioEnabled;
    int lastFreq;
    int lastVolume;
};

/* helpers */

static int radioOn(struct si4708_session *priv)
{
    int ret;

    LOGV("%s: enabling radio", __func__);

    if (priv->radioEnabled) {
        return 0;
    }

    if (!priv->radioInitialised) {
      ret = ioctl(priv->fd, Si4708_IOC_INIT2NORMAL);
      priv->radioInitialised = true;
    } else {
      ret = ioctl(priv->fd, Si4708_IOC_STANDBY2NORMAL);
    }

    if (ret != 0) {
        LOGE("%s: IOCTL Si4708_IOC_INIT2NORMAL failed %d", __func__, ret);
        return -1;
    }

    priv->radioEnabled = true;

    LOGD("FMRadio on");
    return 0;
}

static int radioOff(struct si4708_session *priv)
{
    int ret;

    LOGD("%s: disabling radio radioEnabled=%i", __func__, priv->radioEnabled);

    ret = ioctl(priv->fd, Si4708_IOC_NORMAL2STANDBY);

    if (ret != 0) {
        LOGE("%s: IOCTL Si4708_IOC_NORMAL2STANDBY failed %d", __func__, ret);
        return -1;
    }

    priv->radioEnabled = false;
    LOGD("FMRadio off");

    return 0;
}

static int setFreq(struct si4708_session *priv, int freq)
{
    int ret;

    LOGI("setFreq freq=%d", freq);

    freq = freq / 10;
    ret = ioctl(priv->fd, Si4708_IOC_CHAN_SELECT, &freq);

    if (ret != 0) {
        LOGE("%s: IOCTL Si4708_IOC_CHAN_SELECT failed %d", __func__, ret);
        return -1;
    }

    priv->lastFreq = freq * 10;
    return 0;
}

static int setFreqSpacing(struct si4708_session *priv, int spacing)
{
    int nativeSpacing, ret;

    LOGI("setFreqSpacing spacing=%d", spacing);

    switch (spacing) {
        case 200:
            nativeSpacing = CHAN_SPACING_200_kHz;
            break;
        case 100:
            nativeSpacing = CHAN_SPACING_100_kHz;
            break;
        case 50:
            nativeSpacing = CHAN_SPACING_50_kHz;
            break;
        default:
            LOGE("%s : ERROR invalid Freqency spacing %d", __func__, spacing);
            return -1;
    }

    LOGV("%s: spacing is %d", __func__, nativeSpacing);

    ret = ioctl(priv->fd, Si4708_IOC_CHAN_SPACING_SET, &nativeSpacing);

    if (ret != 0) {
        LOGE("%s: IOCTL Si4708_IOC_CHAN_SPACING_SET failed %d", __func__, ret);
        return FMRADIO_IO_ERROR;
    }

    return FMRADIO_OK;
}

static int setMute(struct si4708_session *priv, int mute)
{
    int ret;
    int zero = 0;

    LOGI("%s: setting mute %d", __func__, mute);

    if (mute) {
        ret = ioctl(priv->fd, Si4708_IOC_VOLUME_GET, &priv->lastVolume);
        ret = ioctl(priv->fd, Si4708_IOC_VOLUME_SET, &zero);
    } else {
        ret = ioctl(priv->fd, Si4708_IOC_VOLUME_SET, &priv->lastVolume);
    }

    if (ret != 0) {
        LOGE("%s: IOCTL Si4708_IOC_MUTE failed failed %d", __func__, ret);
        return FMRADIO_IO_ERROR;
    }

    return FMRADIO_OK;
}

static int setBand(struct si4708_session *priv, int low, int high)
{
    int ret;
    int spacing, de, band;

    LOGI("%s", __func__);

    if (low == 76000 && high == 90000) /* Japan */
        band = BAND_76000_90000_kHz;
    else if ((low == 87500 || low == 87900) /* EU, USA */
            && (high == 107900 || high == 108000))
        band = BAND_87500_108000_kHz;
    else
        band = BAND_76000_108000_kHz;

    LOGI("%s: Setting band %d", __func__, band);

    ret = ioctl(priv->fd, Si4708_IOC_BAND_SET, &band);

    if (ret != 0) {
        LOGE("%s: IOCTL Si4708_IOC_BAND_SET failed %d", __func__, ret);
        return FMRADIO_IO_ERROR;
    }

    return FMRADIO_OK;
}

/* STE FM API implementation */

static int
si4708_rx_start(void **session_data,
                const struct fmradio_vendor_callbacks_t *callbacks,
                int low_freq, int high_freq, int default_freq, int grid)
{
    int res = 0;
    struct si4708_session *priv;
    *session_data = priv = calloc(sizeof(struct si4708_session), 1);

    LOGI("rx_start low_freq=%d high_freq=%d default_freq=%d grid=%d",
            low_freq, high_freq, default_freq, grid);

    priv->lastVolume = 255;
    priv->fd = open("/dev/si4708", O_RDWR);

    res |= radioOn(priv);
    res |= setBand(priv, low_freq, high_freq);
    res |= setFreqSpacing(priv, grid);
    res |= setFreq(priv, default_freq);
    res |= setMute(priv, 0);

    return res;
}

static int
si4708_pause(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    LOGI("pause");

    return setMute(priv, 1);
}

static int
si4708_resume(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    LOGI("resume");

    return setMute(priv, 0);
}

static int
si4708_reset(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int ret = 0;

    LOGI("reset");

    if (priv) {
        ret |= radioOff(priv);
        close(priv->fd);
        free(priv);
        *session_data = 0;
    }

    return ret;
}

static int
si4708_set_frequency(void **session_data, int frequency)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    LOGI("set_frequency frequency=%d", frequency);

    return setFreq(priv, frequency);
}

static int
si4708_get_frequency(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    LOGI("get_frequency frequency=%d", priv->lastFreq);

    return priv->lastFreq;
}

static int
si4708_scan(void **session_data, enum fmradio_seek_direction_t dir)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int val[2];
    int retval;

    LOGI("scan %d", dir);

    if (dir == FMRADIO_SEEK_DOWN) {
        val[0] = SEEKDOWN;
        retval = ioctl(priv->fd, Si4708_IOC_CHAN_SEEK, val);
    } else {
        val[0] = SEEKUP;
        retval = ioctl(priv->fd, Si4708_IOC_CHAN_SEEK, val);
    }

    if (retval != 0) {
        LOGE("Search failed");
        return priv->lastFreq;
    }

    LOGI("%s: freq=%d", __func__, val[1]);
    priv->lastFreq = val[1] * 10;
    return priv->lastFreq;
}

static int
si4708_set_force_mono(void **session_data, int force_mono)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int ret;

    LOGI("%s: setting force mono %d", __func__, force_mono);

    if (force_mono == 0) {
        int stereo = 0;
        ret = ioctl(priv->fd, Si4708_IOC_SET_AUDIOTRACK, &stereo);

        if (ret != 0) {
            LOGE("%s: IOCTL Si4708_IOC_STEREO_SET failed %d", __func__, ret);
            return FMRADIO_IO_ERROR;
        }
    } else {
        int mono = 1;
        ret = ioctl(priv->fd, Si4708_IOC_SET_AUDIOTRACK, &mono);

        if (ret != 0) {
            LOGE("%s: IOCTL Si4708_IOC_MONO_SET failed %d", __func__, ret);
            return FMRADIO_IO_ERROR;
        }
    }

    return FMRADIO_OK;
}

static int
si4708_full_scan(void **session_data, int **found_freqs,
                 int **signal_strenghts)
{
    return FMRADIO_UNSUPPORTED_OPERATION;
}

static int
si4708_stop_scan(void **session_data)
{
    return FMRADIO_OK;
}

int register_fmradio_functions(unsigned int *sig,
                               struct fmradio_vendor_methods_t *funcs)
{
    memset(funcs, 0, sizeof(struct fmradio_vendor_methods_t));

    funcs->rx_start = si4708_rx_start;
    funcs->pause = si4708_pause;
    funcs->resume = si4708_resume;
    funcs->reset = si4708_reset;
    funcs->set_frequency = si4708_set_frequency;
    funcs->get_frequency = si4708_get_frequency;
    funcs->scan = si4708_scan;
    funcs->stop_scan = si4708_stop_scan;
    funcs->set_force_mono = si4708_set_force_mono;
    funcs->full_scan = si4708_full_scan;

    *sig = FMRADIO_SIGNATURE;
    return FMRADIO_OK;
}

