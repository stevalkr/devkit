FROM ros:melodic

USER root
WORKDIR /tmp

ENV USERNAME ubuntu
ENV PASSWORD ubuntu

ENV NVIDIA_VISIBLE_DEVICES ${NVIDIA_VISIBLE_DEVICES:-all}
ENV NVIDIA_DRIVER_CAPABILITIES ${NVIDIA_DRIVER_CAPABILITIES:-all}

RUN sed -i s@/archive.ubuntu.com/@/mirrors.tuna.tsinghua.edu.cn/ubuntu/@g \
      /etc/apt/sources.list && \
    apt-get clean && apt-get update


# install packages
RUN apt-get install -y --no-install-recommends \
      lsb-release software-properties-common gnupg gnupg2 sudo \
      wget curl git build-essential ccache cmake-curses-gui


# install ros
RUN apt-get install -y --no-install-recommends \
      ros-melodic-perception \
      ros-melodic-rviz \
    && apt-get upgrade -y
EXPOSE 11311


# setup user
RUN useradd -m $USERNAME && \
    echo "$USERNAME:$PASSWORD" | chpasswd && \
    usermod --shell /bin/bash $USERNAME && \
    usermod -aG sudo $USERNAME && \
    echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME && \
    usermod  --uid 1000 $USERNAME && \
    groupmod --gid 1000 $USERNAME


# setup dev env
COPY ./etc/setup.sh /
COPY ./etc/entrypoint.sh /
RUN chmod +x /entrypoint.sh /setup.sh && \
    echo "source /setup.sh" >> /home/$USERNAME/.bashrc


# clean
RUN apt-get clean && \
    rm -rf /tmp/* && \
    rm -rf /ros_entrypoint.sh && \
    rm -rf /var/lib/apt/lists/*


# entrypoint
USER $USERNAME
WORKDIR /home/$USERNAME
ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]

