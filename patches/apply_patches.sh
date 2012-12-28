#!/bin/bash

# Set here the base location of your cyanogenmod files
BASE_PATH=~/android/cm7
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

# Apply patches on the framework
applyPatch 0001-Mms-auto-retrieval-for-gingerbread-1-2-framework.patch frameworks_base frameworks/base
applyPatch 0001-WindowManagerService-Set-default-animation-and-trans.patch frameworks_base frameworks/base
applyPatch 0001-Set-default-lockscreen-to-slider.patch frameworks_base frameworks/base

# Must see which works - the first alone does not
applyPatch 0001-Set-beam-animation-default-off.patch frameworks_base frameworks/base
applyPatch 0001-Default-animations-to-fast.patch frameworks_base frameworks/base
applyPatch 0001-Set-fontstyle-small.patch frameworks_base frameworks/base

# Apply patches on CMParts
applyPatch 0001-Set-lockscreen-and-inclass-style-to-slider.patch packages_apps_CMParts packages/apps/CMParts

# Apply patches on Mms
applyPatch 0001-Mms-auto-retrieval-for-gingerbread-2-2-mms-app.patch packages_apps_Mms packages/apps/Mms

# Apply patches on Phone
applyPatch 0001-Set-default-call-ui-to-slider.patch packages_apps_Phone packages/apps/Phone

# Apply patches on vendor/cyanogen
applyPatch 0001-Add-chess-default-wallpaper.patch vendor_cyanogen vendor/cyanogen

