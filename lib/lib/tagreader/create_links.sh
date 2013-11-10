#!/bin/bash
# This script ceates the links to the clementine tagreader

mkdir include
mkdir include/core 
mkdir include/engines 

ln -s ../../daschuer-mixxx-clementine/bin/clementine-tagreader ../../clementine-tagreader
ln -s ../../../../daschuer-mixxx-clementine/ext/libclementine-tagreaderclient/build/libclementine-tagreaderclient.so libclementine-tagreaderclient.so
ln -s ../../daschuer-mixxx-clementine/ext/libclementine-tagreaderclient/build/libclementine-tagreaderclient.so ../../libclementine-tagreaderclient.so

ln -s ../../../../../../daschuer-mixxx-clementine/src/core/tagreaderclient.h include/core/tagreaderclient.h
ln -s ../../../../../../daschuer-mixxx-clementine/src/core/itagreadermetadata.h include/core/itagreadermetadata.h
ln -s ../../../../../../daschuer-mixxx-clementine/src/core/song.h include/core/song.h
ln -s ../../../../../daschuer-mixxx-clementine/bin/src/config.h include/config.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/messagehandler.h include/core/messagehandler.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/workerpool.h include/core/workerpool.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/closure.h include/core/closure.h
ln -s ../../../../../daschuer-mixxx-clementine/bin/ext/libclementine-tagreader/tagreadermessages.pb.h include/tagreadermessages.pb.h
ln -s ../../../../../../daschuer-mixxx-clementine/src/engines/engine_fwd.h include/engines/engine_fwd.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/logging.h include/core/logging.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/messagereply.h include/core/messagereply.h


