FROM ubuntu:20.04
MAINTAINER PeerPlays Blockchain Standards Association

#===============================================================================
# Ubuntu setup
#===============================================================================

RUN \
    apt-get update -y && \
      DEBIAN_FRONTEND=noninteractive apt-get install -y \
      libboost1.67-all-dev \
      libbz2-dev \
      libcurl4-openssl-dev \
      libncurses-dev \
      libreadline-dev \
      libsnappy-dev \
      libssl-dev \
      libtool \
      libzip-dev \
      libzmq3-dev \
      openssh-server \
      sudo \
      wget

ENV HOME /home/peerplays
RUN useradd -rm -d /home/peerplays -s /bin/bash -g root -G sudo -u 1000 peerplays
RUN echo "peerplays  ALL=(ALL) NOPASSWD:ALL" | sudo tee /etc/sudoers.d/peerplays
RUN chmod 440 /etc/sudoers.d/peerplays

RUN service ssh start
RUN echo 'peerplays:peerplays' | chpasswd

# SSH
EXPOSE 22

#===============================================================================
# Peerplays setup
#===============================================================================

ADD build/ /home/peerplays/build

WORKDIR /home/peerplays/peerplays-network

# Setup Peerplays runimage
RUN \
    ln -s /home/peerplays/build/programs/cli_wallet/cli_wallet ./ && \
    ln -s /home/peerplays/build/programs/witness_node/witness_node ./

RUN ./witness_node --create-genesis-json genesis.json && \
    rm genesis.json

RUN chown peerplays:root -R /home/peerplays/peerplays-network

# Peerplays RPC
EXPOSE 8090
# Peerplays P2P:
EXPOSE 9777

# Peerplays
CMD ["./witness_node", "-d", "./witness_node_data_dir"]
