#include "gtest/gtest.h"

#include <QApplication>
#include <QTimer>

GTEST_API_ int main(int argc, char* argv[])
{
  // Ensure that the tests run after QApplication is running its event loop
  // Prevents a crash at shutdown
  // Allows tests that require signals & slots to execute
  QApplication app(argc, argv);

  // Invoke the test suite after exec()
  // Uses lambda to capture argc and argv
  QTimer::singleShot(0, [&]() {
    ::testing::InitGoogleTest(&argc, argv);
    int testResult = RUN_ALL_TESTS();
    app.exit(testResult);
    }
  );

  return app.exec();
}
