#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

#undef unix
struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{        0, "cleanup_module" },
	{        0, "init_module" },
	{        0, "struct_module" },
	{        0, "remove_proc_entry" },
	{        0, "create_proc_entry" },
	{        0, "proc_root" },
	{        0, "put_user_size" },
	{        0, "get_user_size" },
	{        0, "strsep" },
	{        0, "vfree" },
	{        0, "printk" },
	{        0, "vmalloc" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

