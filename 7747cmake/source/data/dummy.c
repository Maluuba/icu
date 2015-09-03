/*
 * This dummy function is linked to the ICU real data library
 * on Windows because we generate the data object file using
 * genccode. The object file generated by genccode won't link
 * on its own because the linker complains that the symbol
 * _DllMainCRTStartup is not defined. That symbol magically
 * becomes defined if we link in the object that the compiler
 * creates from this dummy function.
 */
void dummy(void)
{
}