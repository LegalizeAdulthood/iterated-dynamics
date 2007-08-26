#if !defined(ABSTRACT_INPUT_H)
#define ABSTRACT_INPUT_H

extern int driver_key_pressed();

class AbstractInputContext
{
public:
	virtual ~AbstractInputContext() {}
	virtual bool ProcessWaitingKey(int key) = 0;
	virtual bool ProcessIdle() = 0;
};

class AbstractDialog : public AbstractInputContext
{
public:
	virtual ~AbstractDialog() {}

	void ProcessInput();

protected:
	virtual int driver_key_pressed();
};

#endif
