FROM debian:stable-slim

LABEL org.opencontainers.image.title="Unit (C)"
LABEL org.opencontainers.image.description="Unofficial build of Unit for Docker."
LABEL org.opencontainers.image.url="https://github.com/bandogora/nunit-Ceasy"
LABEL org.opencontainers.image.source="https://github.com/bandogora/nunit-Ceasy"
LABEL org.opencontainers.image.documentation="https://unit.nginx.org/installation/#docker-images"

ARG debug=false

ARG prefix="/usr"
ARG exec_prefix="$prefix"
ARG bin_dir="$exec_prefix/bin"
ARG include_dir="$prefix/include"
ARG lib_dir="$exec_prefix/lib"
ARG data_root_dir="$prefix/share"
ARG man_dir="$data_root_dir/man"
ARG local_state_dir="$prefix/var"
ARG lib_state_dir="$local_state_dir/run/unit"
ARG log_dir="$local_state_dir/log/unit"
ARG log_file="$log_dir/unit.log"
ARG tmp_dir="/tmp"
ARG unit_group="unit"
ARG unit_user="unit"
ARG unit_clone_dir="$prefix/src/unit"

ARG srv_dir="/srv"
ARG app_assets_dir="$srv_dir/assets"
ARG app_bin_dir="$bin_dir"
ARG app_clone_dir="$prefix/src/app"
ARG app_src_dir="$app_clone_dir/src"
ARG app_include_dir="$app_clone_dir/include"
ARG app_lib_dir="$app_clone_dir/lib"
ARG app_group="iots"
ARG app_user="iots"
ARG app_firmware_dir="$srv_dir/firmware"

ARG ncpu="getconf _NPROCESSORS_ONLN"

ENV UNIT_RUN_STATE_DIR="$local_state_dir/run/unit"
ENV UNIT_PID_PATH="$UNIT_RUN_STATE_DIR/unit.pid"
ENV UNIT_SOCKET="$UNIT_RUN_STATE_DIR/control.unit.sock"
ENV UNIT_SBIN_DIR="$exec_prefix/sbin"
ENV DEB_CFLAGS_MAINT_APPEND="-Wp,-D_FORTIFY_SOURCE=2 -march=native -fPIC"
ENV DEB_LDFLAGS_MAINT_APPEND="-Wl,--as-needed -pie"
ENV DEB_BUILD_MAINT_OPTIONS="hardening=+all,-pie"

ARG cc_opt="dpkg-buildflags --get CFLAGS"
ARG ld_opt="dpkg-buildflags --get LDFLAGS"
ARG unit_config_args=\
"--cc=gcc \
--openssl \
--user=$unit_user \
--group=$unit_group \
--prefix=$prefix \
--exec-prefix=$exec_prefix \
--bindir=$bin_dir \
--sbindir=$UNIT_SBIN_DIR \
--includedir=$include_dir \
--libdir=$lib_dir \
--modulesdir=$modules_dir \
--datarootdir=$data_root_dir \
--mandir=$man_dir \
--localstatedir=$local_state_dir \
--logdir=$log_dir \
--log=$log_file \
--runstatedir=$UNIT_RUN_STATE_DIR \
--pid=$UNIT_PID_PATH \
--control=unix:$UNIT_SOCKET \
--tmpdir=$tmp_dir"

# Install dependencies
RUN set -ex \
  && apt-get update \
  && if [ "$debug" = "true" ]; then \
    apt-get install --no-install-recommends --no-install-suggests -y ca-certificates git build-essential \
    libssl-dev libpcre2-dev curl pkg-config vim libcurl4-openssl-dev \
    && echo "alias ls='ls -F --color=auto'" >> /root/.bashrc \
    && echo "alias grep='grep -nI --color=auto'" >> /root/.bashrc; \
  else \
    apt-get install --no-install-recommends --no-install-suggests -y ca-certificates git build-essential \
    libssl-dev libpcre2-dev curl pkg-config libcurl4-openssl-dev; \
  fi

# Prepare dirs, group, and user
RUN set -ex \
  && mkdir -p \
    $bin_dir \
    $UNIT_SBIN_DIR \
    $include_dir \
    $lib_dir \
    $data_root_dir \
    $man_dir \
    $local_state_dir/lib/unit/certs \
    $local_state_dir/lib/unit/scripts \
    $log_dir \
    $UNIT_RUN_STATE_DIR \
    $tmp_dir \
    $app_src_dir \
    $app_include_dir \
    $app_lib_dir \
    $app_clone_dir/config \
    $app_clone_dir/modules/jsmn \
    $app_clone_dir/modules/nibble-and-a-half \
    $app_firmware_dir \
    $app_assets_dir/images \
    $app_assets_dir/javascripts \
    $app_assets_dir/stylesheets \
    /docker-entrypoint.d \
  && mkdir -p -m=700 $lib_state_dir \
  && groupadd --gid 999 $unit_group \
  && groupadd --gid 1000 $app_group \
  && useradd \
       --uid 999 \
       --gid $unit_group \
       --no-create-home \
       --home /nonexistent \
       --comment "unit user" \
       --shell /bin/false \
       $unit_user \
  && useradd \
       --uid 1000 \
       --gid $app_group \
       --no-create-home \
       --home /nonexistent \
       --comment "iot server app user" \
       --shell /bin/false \
       $app_user \
  && chown root:$app_group $app_firmware_dir \
  && chmod -R 775 $app_firmware_dir

# Clone Unit
RUN set -ex && git clone --depth 1 -b 1.32.1-1 https://github.com/nginx/unit $unit_clone_dir

WORKDIR $unit_clone_dir

# Configure, make, and install unitd
RUN set -ex \
  && if [ "$debug" = "true" ]; \
    then ./configure $unit_config_args --debug --cc-opt="$(eval $cc_opt)" --ld-opt="$(eval $ld_opt)"; \
    else ./configure $unit_config_args --cc-opt="$(eval $cc_opt)" --ld-opt="$(eval $ld_opt)"; \
  fi \
  && make -j $(eval $ncpu) unitd \
  && install -pm755 ./build/sbin/unitd "$UNIT_SBIN_DIR/unitd" \
  && ln -sf /dev/stdout "$log_file" \
  && make -j $(eval $ncpu) libunit-install \
# Add nxt_clang.h to include location
  && cp ./src/nxt_clang.h $include_dir/.

# Save/apt-mark unitd dependencies
RUN set -ex \
  && cd \
  && savedAptMark="$(apt-mark showmanual)" \
  && for f in $UNIT_SBIN_DIR/unitd; do \
      ldd $f | awk '/=>/{print $(NF-1)}' | while read n; do dpkg-query -S $n; done | sed 's/^\([^:]\+\):.*$/\1/' | sort | uniq >> /requirements.apt; \
     done \
  && apt-mark showmanual | xargs apt-mark auto > /dev/null \
  && { [ -z "$savedAptMark" ] || apt-mark manual $savedAptMark; } \
  && /bin/true \
  && apt-get update \
  && apt-get --no-install-recommends --no-install-suggests -y install $(cat /requirements.apt)

WORKDIR $app_clone_dir

# Copy app source files
COPY --link ./src/* "$app_src_dir"
COPY --link ./include/* "$app_include_dir"
COPY --link ./Makefile "$app_clone_dir"
COPY --link ./modules/jsmn/* "$app_clone_dir"/modules/jsmn
COPY --link ./modules/nibble-and-a-half/* "$app_clone_dir"/modules/nibble-and-a-half
COPY --link ./assets/*.html "$app_assets_dir"
COPY --link ./assets/images/* "$app_assets_dir/images"
COPY --link ./assets/javascripts/* "$app_assets_dir/javascripts"
COPY --link ./assets/stylesheets/* "$app_assets_dir/stylesheets"

RUN --mount=type=secret,id=vzw_secrets.h set -x \
  && cp /run/secrets/vzw_secrets.h config/vzw_secrets.h

# Compile and link C app against libunit.a
Run set -x \
  && make -j $(eval $ncpu) \
  && make install \
  && rm -f config/vzw_secrets.h \
  && cd \
  && apt-get purge -y --auto-remove build-essential \
  && rm -rf /usr/src/* \
  && rm -rf /var/lib/apt/lists/* \
  && rm -f /requirements.apt

# Copy initial config
COPY --link config/config.json /docker-entrypoint.d/

# Copy the default entrypoint
COPY --link ./docker-entrypoint.sh /usr/local/bin/

# Make sure the entrypoint is executable
RUN ["chmod", "+x", "/usr/local/bin/docker-entrypoint.sh"]

STOPSIGNAL SIGTERM

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
EXPOSE 80
CMD ["unitd", "--no-daemon", "--control", "unix:$UNIT_SOCKET"]
