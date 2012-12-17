# LatinIME: Option to reverse volume key logic on cursor control
#cd ~/android/cm9/packages/inputmethods/LatinIME
#git reset --hard
# Get patch and apply it
#git fetch http://review.cyanogenmod.com/CyanogenMod/android_packages_inputmethods_LatinIME refs/changes/26/16826/4 && git format-patch -1 --stdout FETCH_HEAD > ~/patch.patch
#git apply ~/patch.patch
#rm ~/patch.patch

# Email: Add option to allow the download/upload of "potentially" insecure attachment
cd ~/android/cm9/packages/apps/Email
git am 0001-Email-insecure-attachments.patch

# Mms auto-retrieval
cd ~/android/cm9/frameworks/base
git am 0001-Mms-auto-retrieval-ics-1-2-framework.patch
cd ~/android/cm9/packages/apps/Mms
git am 0001-Mms-auto-retrieval-ics-2-2-mms.patch

# Vendor
cd ~/android/cm9/vendor/cm
git am 0001-Add-blade-to-lunch-menu.patch
git am 0001-Remove-cm-bloat.patch

# Build
cd ~/android/cm9/build
git am 0001-Remove-tts-and-videoeditor.patch

# QCOM_LEGACY_OMX
cd ~/android/cm9/frameworks/base
git am 0001-Add-support-for-QCOM_LEGACY_OMX.patch

cd ~/android/cm9/