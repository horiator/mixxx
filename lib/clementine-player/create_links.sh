#!/bin/bash
# This script ceates the links to the clementine Player headers and libraries 

ln -s ../../../../daschuer-mixxx-clementine/src src
ln -s ../../../../daschuer-mixxx-clementine/bin/src/libclementine_lib.a libclementine_lib.a
ln -s ../../../../daschuer-mixxx-clementine/bin/ext/libclementine-common/liblibclementine-common.a liblibclementine-common.a
ln -s ../../../../daschuer-mixxx-clementine/bin/3rdparty/sha2/libsha2.a libsha2.a
ln -s ../../../../daschuer-mixxx-clementine/bin/3rdparty/universalchardet/libchardet.a libchardet.a
ln -s ../../../../daschuer-mixxx-clementine/bin/ext/libclementine-spotifyblob/libclementine-spotifyblob-messages.a libclementine-spotifyblob-messages.a
ln -s ../../../../daschuer-mixxx-clementine/bin/3rdparty/chromaprint/src/libchromaprint_p.a libchromaprint_p.a
ln -s ../../../../daschuer-mixxx-clementine/bin/3rdparty/qtiocompressor/libqtiocompressor.a libqtiocompressor.a
ln -s ../../../../daschuer-mixxx-clementine/bin/gst/afcsrc/libgstafcsrc.a libgstafcsrc.a
ln -s ../../../../daschuer-mixxx-clementine/bin/ext/libclementine-tagreader/liblibclementine-tagreader.a liblibclementine-tagreader.a
ln -s ../../../../daschuer-mixxx-clementine/bin/3rdparty/libmygpo-qt/libmygpo-qt.a libmygpo-qt.a

mkdir include 
mkdir include/core
ln -s ../../../../../daschuer-mixxx-clementine/bin/src/config.h include/config.h
ln -s ../../../../../daschuer-mixxx-clementine/bin/ext/libclementine-tagreader/tagreadermessages.pb.h include/tagreadermessages.pb.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/messagehandler.h include/core/messagehandler.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/logging.h include/core/logging.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/messagereply.h include/core/messagereply.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/workerpool.h include/core/workerpool.h
ln -s ../../../../../../daschuer-mixxx-clementine/ext/libclementine-common/core/closure.h include/core/closure.h

ln -s ../../daschuer-mixxx-clementine/bin/clementine-tagreader ../../clementine-tagreader

