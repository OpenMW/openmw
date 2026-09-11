FROM ubuntu:22.04

ENV DEBIAN_FRONTEND="noninteractive" \
    HOME="/home/docs"

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        curl \
        git \
        libncurses-dev \
        libreadline-dev \
        python3 \
        python3-pip \
        unzip \
        wget \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /home/docs \
    && touch /home/docs/omw_luadoc_docker

WORKDIR /home/docs

COPY requirements.txt ./
RUN python3 -m pip install --no-cache-dir -r requirements.txt

COPY source/install_luadocumentor_in_docker.sh ./
RUN bash -e ./install_luadocumentor_in_docker.sh

VOLUME /openmw