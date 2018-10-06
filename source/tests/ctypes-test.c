#include <assert.h>
#include <stdio.h>

typedef void (*PythonSideFn)(int checker);

PythonSideFn pyFn = NULL;

__attribute__ ((visibility("default")))
void set_python_side_fn(PythonSideFn fn)
{
    printf("set_python_side_fn(%p)\n", fn);
    pyFn = fn;
}

__attribute__ ((visibility("default")))
void call_python_side_fn(void)
{
    assert(pyFn != NULL);

    printf("going to call python function now, ptr: %p\n", pyFn);
    pyFn(1337);
    printf("done!\n");
}
