#ifndef D_Stack_H
#define D_Stack_H

///////////////////////////////////////////////////////////////////////////////
//
//  Stack is responsible for ...
//
///////////////////////////////////////////////////////////////////////////////

class Stack
  {
  public:
    explicit Stack();
    virtual ~Stack();
    
    bool IsEmpty();
    void Push(int);
    int Pop();

  private:
    int index;
    int values[10];
    
    Stack(const Stack&);
    Stack& operator=(const Stack&);
  };

#endif  // D_Stack_H
