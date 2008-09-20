#include "UnitTestHarness/TestHarness.h"
#include "Helpers/SimpleStringExtensions.h"
#include "Stack.h"

// to make sure this file gets linked in to your test main
EXPORT_TEST_GROUP(Stack);

namespace
  {
  Stack* stack;

  void SetUp()
  {
    stack = new Stack();
  }
  void TearDown()
  {
    delete stack;
  }
}

TEST(Stack, Create)
{
}

TEST(Stack, NotEmpty)
{
    stack->Push(12);
    CHECK(!stack->IsEmpty());
}

TEST(Stack, PopReturnsWhatWasPushed)
{
    stack->Push(12);
    LONGS_EQUAL(12, stack->Pop());
}

TEST(Stack, PushOnePopOneEmpty)
{
    stack->Push(12);
    stack->Pop();
    CHECK(stack->IsEmpty());
}

TEST(Stack, PushPopGeneralCase)
{
    stack->Push(10);
    stack->Push(11);
    stack->Push(12);
    LONGS_EQUAL(12, stack->Pop());
    CHECK(!stack->IsEmpty());
    LONGS_EQUAL(11, stack->Pop());
    CHECK(!stack->IsEmpty());
    LONGS_EQUAL(10, stack->Pop());
    CHECK(stack->IsEmpty());
}

