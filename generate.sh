#!/bin/bash

PREMAKE_URL='https://github.com/premake/premake-core/releases/download/v5.0.0-beta4/premake-5.0.0-beta4-linux.tar.gz'
PREMAKE_HASH='4356ab7cdec6085183d68fb240089376eacdc2fb751ffbd8063d797ae43abeb3'

# The following variables can be set:
#     PREMAKE_NO_GLOBAL - Ignore premake5 executable from path
#     PREMAKE_NO_PROMPT - Download premake5 without prompting

function install_premake {
    if [[ ! -x "$(command -v wget)" ]]; then
        echo "Failed: Installation requires wget" >&2
        exit 2
    fi
    if [[ ! -x "$(command -v tar)" ]]; then
        echo "Failed: Installation requires tar" >&2
        exit 2
    fi
    if [[ ! -x "$(command -v sha256sum)" ]]; then
        echo "Failed: Installation requires sha256sum" >&2
        exit 2
    fi

    mkdir -p build
    wget -nd -O build/premake.tar.gz "$PREMAKE_URL"
    if [[ $? -ne 0 ]]; then
        echo "Download failed" >&2
        exit 2
    fi

    tar -xf build/premake.tar.gz -C build
    if [[ $? -ne 0 ]]; then
        echo "Extraction failed" >&2
        exit 2
    fi

    rm build/premake.tar.gz

    echo "${PREMAKE_HASH} build/premake5" | sha256sum -c
    if [[ $? -ne 0 ]]; then
        echo "Hash verification failed" >&2
        rm build/premake5
        exit 2
    fi

    chmod +x build/premake5
}

# Go to repository root
cd "$(dirname "$0")" || exit 2

if [[ ! -d ".git" ]]; then
    echo "You must clone the OpenAssetTools repository using 'git clone'. Please read README.md." >&2
    exit 1
fi

PREMAKE_BIN=''
if [[ -z "$PREMAKE_NO_GLOBAL" ]] && [[ -x "$(command -v premake5)" ]]; then
    PREMAKE_BIN='premake5'
elif [[ -x "$(command -v build/premake5)" ]]; then
    PREMAKE_BIN='build/premake5'
else
    echo "Could not find premake5. You can either install it yourself or this script download it for you."
    if [[ ! -z "$PREMAKE_NO_PROMPT" ]] || [[ "$(read -e -p 'Do you wish to download it automatically? [y/N]> '; echo $REPLY)" == [Yy]* ]]; then
        echo "Installing premake"
        install_premake
        PREMAKE_BIN='build/premake5'
    else
        echo "Please install premake5 and try again"
        exit 1
    fi
fi

git submodule update --init --recursive
$PREMAKE_BIN $@ gmake2
