/*
   When builing with GCC in the Github windows executer,
   libelf sems to require the stack protector.
   To allow the linking to succeed, we just add a dummy definition
   for the stack protection hooks to the source.
   /TODO: remove this workaround
 */

void __stack_chk_fail(void) {}
void *__stack_chk_guard = 0;
