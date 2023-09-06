FROM etherswangel/ros:noetic

USER root
WORKDIR /tmp


# setup repos
RUN add-apt-repository ppa:neovim-ppa/unstable && \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update


# install clang
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 16 all && \
    ln -s /usr/bin/clang-16 /usr/bin/clang && \
    ln -s /usr/bin/clang++-16 /usr/bin/clang++ && \
    ln -s /usr/bin/clangd-16 /usr/bin/clangd && \
    ln -s /usr/bin/clang-tidy-16 /usr/bin/clang-tidy && \
    ln -s /usr/bin/clang-format-16 /usr/bin/clang-format


# install neovim
RUN apt-get install -y --no-install-recommends \
      neovim python3-neovim xclip mlocate \
      codespell ripgrep fd-find \
      libstdc++6 libomp-16-dev && \
    apt-get dist-upgrade -y


# etc
COPY ./etc /home/$USERNAME/etc
RUN echo "source ~/etc/bashrc" >> /home/$USERNAME/.bashrc


# clean
RUN apt-get clean && \
    rm -rf /tmp/* && \
    rm -rf /var/lib/apt/lists/*


# entrypoint
USER $USERNAME
WORKDIR /home/$USERNAME

