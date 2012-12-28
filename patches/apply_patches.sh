#!/bin/bash

# Set here the base location of your cyanogenmod files
BASE_PATH=~/android/cm9
PATCHES_PATH=$BASE_PATH/device/zte/blade/patches

# Copies the patch to a location and applies 'git am'
function applyPatch {
    # Paths
    local SRC_FILE=$PATCHES_PATH/$2/$1
    local DST_PATH=$BASE_PATH/$3
    local DST_FILE=$DST_PATH/$1
    # Copy src_file to dst_path
    cp $SRC_FILE $DST_PATH
    # Apply the patch
    cd $DST_PATH
    git am $DST_FILE
    # Remove the patch file
    rm $DST_FILE
    # Return to patches location
    cd $PATCHES_PATH
}

# Email: Add option to allow the download/upload of "potentially" insecure attachment
applyPatch 0001-Email-insecure-attachments.patch packages_apps_Email packages/apps/Email

# Mms auto-retrieval
applyPatch 0001-Mms-auto-retrieval-ics-1-2-framework.patch frameworks_base frameworks/base
applyPatch 0001-Mms-auto-retrieval-ics-2-2-mms.patch packages_apps_Mms packages/apps/Mms

# Vendor
applyPatch 0001-Add-blade-to-lunch-menu.patch vendor_cm vendor/cm
applyPatch 0001-Remove-cm-bloat.patch vendor_cm vendor/cm
applyPatch 0001-Add-EcoWallpapers-to-build.patch vendor_cm vendor/cm

# Build
applyPatch 0001-Remove-tts-and-videoeditor.patch build build

# QCOM_LEGACY_OMX and Fix video Thumbnails
applyPatch 0001-Add-support-for-QCOM_LEGACY_OMX.patch frameworks_base frameworks/base
applyPatch 0001-Fix-video-thumbnail-generation.patch frameworks_base frameworks/base

# Remove Trebuchet wallpapers
applyPatch 0001-Trebuchet-Remove-wallpapers.patch packages_apps_Trebuchet packages/apps/Trebuchet

