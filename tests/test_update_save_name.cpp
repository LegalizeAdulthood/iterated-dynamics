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

    EXPECT_EQ(std::string{"myfrac1.gif"}, filename);
}

TEST(TestUpdateSaveName, singleNumberAtEnd)
{
    char filename[1024];
    strcpy(filename, "myfrac1.gif");
    
    update_save_name(filename);

    EXPECT_EQ(std::string{"myfrac2.gif"}, filename);
}

TEST(TestUpdateSaveName, nineAtEnd)
{
    char filename[1024];
    strcpy(filename, "myfrac9.gif");
    
    update_save_name(filename);

    EXPECT_EQ(std::string{"myfra10.gif"}, filename);
}
