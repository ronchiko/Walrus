
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <walrus/parsing.h>

#define MAX_BUFFER_SIZE 	64

#define INV_CONST_ERR(type, msg) \
						Walrus_RaiseError(WALRUS_ERR_INVALID_CONSTANT);		\
						Walrus_ErrorPushParam(0, type, false);			\
						Walrus_ErrorPushParam(1, Walrus_strdup(begin), true); \
						Walrus_ErrorPushParam(2, msg, false);

typedef struct  {
	char buffer[MAX_BUFFER_SIZE];
	int current;
} Walrus_BufferStack;

static bool Walrus_StackPush(Walrus_BufferStack *stack, char value) {
	if(stack->current >= MAX_BUFFER_SIZE) return false;

	stack->buffer[stack->current++] = value;
	return true;
}

static int Walrus_StackToInt(Walrus_BufferStack *stack, int _default) {
	if (!stack->current) return _default;

	int mul = 1, integer = 0;
	while(stack->current) {
		integer += mul * (stack->buffer[--stack->current] - '0');
		mul *= 10;
	}

	return integer;
}

static float Walrus_Pow10(int exp) {
	float base = 1;

	int sign = exp < 0 ? -1 : 1;
	float amplifier = sign < 0 ? .1f : 10.f;

	for(exp *= sign;exp;exp--)
		base *= amplifier;

	return base;
}

float Walrus_ConvertToFloat(const char *buffer) {
	
	enum {
		INTEGER = 0, 
		FRACTION = 1, 
		EXPONENT = 2
	} state = INTEGER;
	const char *begin = buffer;

	Walrus_BufferStack stacks[3] = {
		[INTEGER] 	= { { 0 }, 0 },
		[FRACTION] 	= { { 0 }, 0 },
		[EXPONENT]	= { { 0 }, 0 }
	};

	bool invertExp = false;
	bool invertInt = false; 

	for(;*buffer;++buffer) {
		switch (*buffer)
		{
		case '.':
			if (state != INTEGER) {
				INV_CONST_ERR("float", "only one period allowed per decimal number");
				return 0;
			}
			state = FRACTION;
			break;
		case 'E':
		case 'e':
			if(state == EXPONENT) {
				INV_CONST_ERR("float", "only one exponent allowed per decimal number");
				return 0;
			}
			state = EXPONENT;
			break;
		case '-':
			if((stacks + state)->current) {
				INV_CONST_ERR("float", "minus only allowed before constant");
				return 0;
			}

			if (state == INTEGER) 		invertInt = true;
			else if (state == EXPONENT)	invertExp = true;
			else {
				INV_CONST_ERR("float", "cannot invert fraction");
				return 0;
			}
			break;
		default:
			if (isdigit(*buffer)) {
				Walrus_StackPush(stacks + state, *buffer);
				continue;
			}

			INV_CONST_ERR("float", "invalid character");
			return 0;
		}
	}

	int factionDepth = stacks[FRACTION].current;

	int integer  = Walrus_StackToInt(stacks + INTEGER,  0);
	int fraction = Walrus_StackToInt(stacks + FRACTION, 0);
	int exponent = Walrus_StackToInt(stacks + EXPONENT, 0);

	if (invertInt) {
		integer  *= -1;
		fraction *= -1; 
	}

	if(invertExp) exponent *= -1;

	float fFraction = (float)fraction / Walrus_Pow10(factionDepth);

	return (integer + fFraction) * Walrus_Pow10(exponent);
}

int Walrus_ConvertToInt(const char *buffer) {
	Walrus_BufferStack stack = { { 0 }, 0 };
	const char *begin = buffer;

	bool invert = false;
	if(*buffer == '-') {
		invert = true; 
		++buffer;
	} 

	for(;*buffer;++buffer) {
		if(!isdigit(*buffer)){
			INV_CONST_ERR("int", "invalid character");
			return 0;
		}

		Walrus_StackPush(&stack, *buffer);
	}

	return Walrus_StackToInt(&stack, 0)  * (invert ? -1 : 1);
}

int Walrus_ConvertHex(const char *buffer) {

	const char *begin = buffer;
	const int MAX_ITER = sizeof(int) << 1;

	Walrus_BufferStack stack = { { 0 }, 0 };
	for(int i = 0; i < MAX_ITER && *buffer; ++i, ++buffer) {
		char current = tolower(*buffer);
		if(!('a' <= current && current <= 'f') && !isdigit(current)) {
			INV_CONST_ERR("int", "invalid character");
			return 0;
		}

		Walrus_StackPush(&stack, current);
	}

	int value = 0;
	const int length = stack.current - 1;
	while(stack.current) {
		const char digit = stack.buffer[--stack.current];
		const int shift = (length - stack.current) * 4;

		if (isdigit(digit)) value |= (digit - '0') << shift;
		else if('a' <= digit && digit <= 'f') value |= ((digit - 'a') + 0xA) << shift;
	}

	return value;
}

int Walrus_ConvertBinary(const char *buffer) {
	const char *begin = buffer;
	const int MAX_ITER = sizeof(int) << 3;

	Walrus_BufferStack stack= { { 0 }, 0 };
	for(int i = 0;i < MAX_ITER && *buffer; ++i, ++buffer) {
		if('0' != *buffer && '1' != *buffer) {
			INV_CONST_ERR("int", "invalid character");
			return 0;
		}

		Walrus_StackPush(&stack, *buffer);
	}

	int value = 0;
	const int length = stack.current - 1;
	while(stack.current) {
		const char digit = stack.buffer[--stack.current];
		const int shift = length - stack.current;

		value |= (digit - '0') << shift;
	}

	return value;
}

Walrus_Type Walrus_ConvertNumeric(const char *buffer, void *out) {

	if (*buffer == '.')	{
		*(float*)out = Walrus_ConvertToFloat(buffer);
		if(Walrus_HasError()) return WALRUS_ERROR;
		return WALRUS_DECIMAL;
	}

	if (*buffer == '0') {
		switch (buffer[1])
		{
		case 'x':
			*(int *)out = Walrus_ConvertHex(buffer + 2);
			return WALRUS_INTEGER;
		case 'b':
			*(int *)out = Walrus_ConvertBinary(buffer + 2);
			if(Walrus_HasError()) return WALRUS_ERROR;
			return WALRUS_INTEGER;
		case '.':
			*(float*)out = Walrus_ConvertToFloat(buffer);
			if(Walrus_HasError()) return WALRUS_ERROR;
			return WALRUS_DECIMAL;
		}
	}

	int i = Walrus_ConvertToInt(buffer);
	if(!Walrus_HasError()) {
		*(int *)out = i;
		return WALRUS_INTEGER;
	}

	Walrus_ClearError();
	float f = Walrus_ConvertToFloat(buffer);
	if(!Walrus_HasError()) {
		*(float *)out = f;
		return WALRUS_DECIMAL;
	}

	const char *begin = buffer;
	INV_CONST_ERR("numeric", "failed to parse numeric");
	return WALRUS_ERROR;
}