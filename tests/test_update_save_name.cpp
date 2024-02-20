#include <update_save_name.h>

#include <gtest/gtest.h>

TEST(TestUpdateSaveName, basic)
{
    char filename[1024];
    strcpy(filename, "fract0001.gif");
    
    update_save_name(filename);

    EXPECT_EQ(std::string{"fract0002.gif"}, filename);
}

TEST(TestUpdateSaveName, noNumbersAtEnd)
{
    char filename[1024];
    strcpy(filename, "myfract.gif");
    
    update_save_name(filename);

    EXPECT_EQ(std::string{"myfract2.gif"}, filename);
}

TEST(TestUpdateSaveName, singleNumberAtEnd)
{
    char filename[1024];
    strcpy(filename, "myfract1.gif");
    
    update_save_name(filename);

    EXPECT_EQ(std::string{"myfract2.gif"}, filename);
}

TEST(TestUpdateSaveName, nineAtEnd)
{
    char filename[1024];
    strcpy(filename, "myfract9.gif");
    
    update_save_name(filename);

    EXPECT_EQ(std::string{"myfract10.gif"}, filename);
}

TEST(TestNextSaveName, basic)
{
    EXPECT_EQ("fract0002.gif", next_save_name("fract0001.gif"));
}
