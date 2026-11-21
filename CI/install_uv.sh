#!/bin/bash

set -euo pipefail

UV_VERSION=0.13.8
UV_TARGET=x86_64-unknown-linux-musl
UV_SHA256=61fb26dabc4f0cf1d050fc6c67d40e43e4fae2994af21ebc1e050b233bf2b5ed

cd "$(mktemp -d)"
curl -fsSL --retry 3 "https://github.com/astral-sh/uv/releases/download/${UV_VERSION}/uv-${UV_TARGET}.tar.gz" -o uv.tar.gz
echo "${UV_SHA256}  uv.tar.gz" | sha256sum -c
tar -xzf uv.tar.gz --no-same-owner --strip-components=1 -C /usr/local/bin "uv-${UV_TARGET}/uv"
uv --version
