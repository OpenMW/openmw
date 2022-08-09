FROM readthedocs/build:latest
ENV HOME="/home/docs"
RUN touch /home/docs/omw_luadoc_docker

COPY requirements.txt ./
RUN pip3 install -r requirements.txt

COPY source/install_luadocumentor_in_docker.sh ./
RUN bash ./install_luadocumentor_in_docker.sh

VOLUME /openmw
