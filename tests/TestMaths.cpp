#include "TestMaths.h"

void TestMaths::toUpper()
{
        QString str = "Hello";
            QCOMPARE(str.toUpper(), QString("HELLO"));
}

QTEST_MAIN(TestMaths)

