#include <gtest/gtest.h>
#include <qapplication.h>
#ifdef __TAGREADER__
#include <core/logging.h>
#endif // __TAGREADER__

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
#ifdef __TAGREADER__
    logging::Init();
#endif // __TAGREADER__
    QApplication a(argc, argv);
    return RUN_ALL_TESTS();
}
