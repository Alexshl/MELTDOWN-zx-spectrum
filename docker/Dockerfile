# All stages pinned to linux/amd64.
# On Apple Silicon (arm64) Docker Desktop runs this via Rosetta 2 transparently.
# See docker/README.md for details.

# Stage 1: z88dk from official upstream Alpine image.
# The binaries link against musl libc + libstdc++ (Alpine/musl flavour).
# We copy both the toolchain AND its musl runtime into the final Ubuntu stage.
FROM --platform=linux/amd64 z88dk/z88dk:latest AS z88dk

# Stage 2: build ZEsarUX from source on Ubuntu 24.04
FROM --platform=linux/amd64 ubuntu:24.04 AS zesarux-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        git \
        gcc \
        make \
        libncurses-dev \
        libssl-dev \
        libsdl2-dev \
        libpulse-dev \
        libsndfile1-dev \
        libasound2-dev \
        pkg-config \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
RUN git clone --depth 1 --branch ZEsarUX-12.1 \
        https://github.com/chernandezba/zesarux.git /src/zesarux

WORKDIR /src/zesarux/src
RUN ./configure \
        --enable-sdl2 \
        --disable-xwindows \
        --disable-aa \
        --disable-caca \
        --disable-fbdev \
        --prefix=/usr/local \
    && make -j$(nproc) \
    && make install

# Stage 3: final runtime image on Ubuntu 24.04
FROM --platform=linux/amd64 ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Runtime deps for ZEsarUX and general tooling.
# m4: required by zcc as a preprocessor at runtime.
# No musl package needed: we copy the full musl runtime from the Alpine z88dk stage.
RUN apt-get update && apt-get install -y --no-install-recommends \
        make \
        gcc \
        libc6-dev \
        m4 \
        libsdl2-2.0-0 \
        libncurses6 \
        libssl3 \
        libpulse0 \
        libsndfile1 \
        libasound2t64 \
        z80dasm \
        ca-certificates \
        python3 \
        file \
        xxd \
        bsdextrautils \
    && rm -rf /var/lib/apt/lists/*

# Copy z88dk toolchain from upstream Alpine image
ENV Z88DK=/opt/z88dk
COPY --from=z88dk /opt/z88dk $Z88DK

# Copy musl runtime libs that z88dk binaries were compiled against (Alpine musl flavour).
# Full dependency list from: ldd /opt/z88dk/bin/* | grep "=>" | sort -u
# Placing in /usr/lib/x86_64-linux-musl keeps them separate from glibc libs.
COPY --from=z88dk /lib/ld-musl-x86_64.so.1    /lib/ld-musl-x86_64.so.1
COPY --from=z88dk /lib/libc.musl-x86_64.so.1  /lib/libc.musl-x86_64.so.1
COPY --from=z88dk /usr/lib/libstdc++.so.6      /usr/lib/x86_64-linux-musl/libstdc++.so.6
COPY --from=z88dk /usr/lib/libgcc_s.so.1       /usr/lib/x86_64-linux-musl/libgcc_s.so.1
COPY --from=z88dk /usr/lib/libgmp.so.10        /usr/lib/x86_64-linux-musl/libgmp.so.10
COPY --from=z88dk /usr/lib/liblzma.so.5        /usr/lib/x86_64-linux-musl/liblzma.so.5
COPY --from=z88dk /usr/lib/libxml2.so.2        /usr/lib/x86_64-linux-musl/libxml2.so.2
COPY --from=z88dk /usr/lib/libz.so.1           /usr/lib/x86_64-linux-musl/libz.so.1

# Patch musl's library search path.
# musl reads /etc/ld-musl-x86_64.path (colon-separated dirs) at runtime.
RUN echo '/lib:/usr/lib/x86_64-linux-musl:/usr/lib' \
        > /etc/ld-musl-x86_64.path

ENV PATH=$Z88DK/bin:/usr/local/bin:$PATH
ENV ZCCCFG=$Z88DK/lib/config

# Copy ZEsarUX binary and data
COPY --from=zesarux-builder /usr/local/bin/zesarux* /usr/local/bin/
# Ensure 'zesarux' is available regardless of pthread-suffixed binary name
RUN ls /usr/local/bin/zesarux* 2>/dev/null | head -1 | \
    xargs -I{} sh -c '[ -f /usr/local/bin/zesarux ] || ln -s {} /usr/local/bin/zesarux'

COPY --from=zesarux-builder /usr/local/share/zesarux /usr/local/share/zesarux

# Headless SDL: no display needed for ZRCP / trace / disasm workflows
ENV SDL_VIDEODRIVER=dummy
ENV SDL_AUDIODRIVER=dummy

# Non-root user as default; caller passes --user $(id -u):$(id -g) for correct artifact ownership.
# Ubuntu 24.04 already has UID 1000 (ubuntu user), so use 1001 for dev.
RUN useradd -m -u 1001 dev
USER dev
WORKDIR /src

COPY --chmod=0755 docker/entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
CMD ["build"]
