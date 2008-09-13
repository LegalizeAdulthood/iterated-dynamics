#if !defined(ABSTRACT_INPUT_H)
#define ABSTRACT_INPUT_H

class AbstractInputContext
{
public:
	virtual ~AbstractInputContext() {}
	virtual bool ProcessWaitingKey(int key) = 0;
	virtual bool ProcessIdle() = 0;
};

class AbstractDriver;

class AbstractDialog : public AbstractInputContext
{
public:
	AbstractDialog(AbstractDriver *driver);
	virtual ~AbstractDialog() {}

	void ProcessInput();

protected:
	AbstractDriver *_driver;
};

#endif
