export BUILD_ROOT=$(shell pwd)

export INCLUDE_PATH=$(BUILD_ROOT)/_include

BUILD_DIR=$(BUILD_ROOT)/parsestr/ \
          $(BUILD_ROOT)/proc/ \
		  $(BUILD_ROOT)/app/

export DEBUG=true
