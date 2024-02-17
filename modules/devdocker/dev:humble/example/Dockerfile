FROM etherswangel/dev:humble

USER root


# install deps
RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y --no-install-recommends \
      libyaml-cpp-dev


# entrypoint
USER $USERNAME
WORKDIR /home/$USERNAME

