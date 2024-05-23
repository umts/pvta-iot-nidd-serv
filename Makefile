CC ?= gcc
DIR ?= .
app_bin_dir ?= /usr/bin

CFLAGS ?= -O3 -fstack-protector-strong -Wall -Wextra -Wformat -Winline -Werror=format-security \
	-D_FORTIFY_SOURCE=2 -march=native -fPIC -I$(DIR)/include
LDFLAGS ?= -Wl,-z,relro -Wl,-z,now -Wl,--as-needed -pie -L$(DIR) -L$(DIR)/lib -lc -lpthread -lcurl -lunit -lnaah64

VPATH = $(DIR)/src:$(DIR)/include

objects = main.o jsmn.o json_helpers.o http_get_stop.o curl_callbacks.o \
	parse_stop_json.o vzw_connect.o request_handler.o firmware_requests.o

all: jsmn.h base64.h libnaah64.a app

app: config/vzw_secrets.h $(objects)
	$(CC) $(CFLAGS) $(objects) -o $(DIR)/app $(LDFLAGS)
	chmod +x $(DIR)/app

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: clean jsmn.h base64.h libnaah64.a install

libnaah64.a:
	$(MAKE) -C $(DIR)/modules/nibble-and-a-half libnaah64.a
	cp $(DIR)/modules/nibble-and-a-half/libnaah64.a $(DIR)/lib/

base64.h:
	cp $(DIR)/modules/nibble-and-a-half/base64.h $(DIR)/include

jsmn.h:
	cp $(DIR)/modules/jsmn/jsmn.h $(DIR)/include

install: app
	cp $(DIR)/app $(app_bin_dir)/iots

clean:
	rm -rf $(DIR)/app $(objects) $(DIR)/lib/libnaah64.a $(DIR)/include/jsmn.h $(DIR)/include/base64.h
	$(MAKE) -C $(DIR)/modules/nibble-and-a-half clean
