void foo()
{
	if (myBoolean) {
#ifdef DEBUG
		printf("ACK");
#endif
	}
	return true;
}

void foo2()
{
	if (m_errno == ERR_NONE) {
		function1(variables);
		function2(variables);
	} else {
		function1(variables);
	}
	//MyComment1
	//MyComment2
}

void foo3()
{
	if (statement) {
		if (statement) {
			condition;
			return true;
		}
	}
	return false;
}
