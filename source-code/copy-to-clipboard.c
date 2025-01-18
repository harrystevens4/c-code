#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <unistd.h>
GMainLoop *mainloop;
int set_clipboard(const char *text);
void clipboard_set_callback(GObject *source, GAsyncResult *res, gpointer data){
	GError *err = NULL;
	if (gdk_clipboard_store_finish((GdkClipboard *)source,res,&err)){
		printf("success\n");
	}else{
		printf("failure\n");
		printf("%s\n",err->message);
	}
	g_main_loop_quit(mainloop);
	g_error_free(err);
}
int main(int argc, char *argv){
	set_clipboard("abacus");
}
int set_clipboard(const char *text){
	mainloop = g_main_loop_new(NULL,FALSE);
	int return_val = 0;
	GApplication *app = g_application_new("com.github.rufus173.copyToClipboard",0);

	// open display
	GdkDisplay *display = gdk_display_get_default();
	if (display == NULL){
		//no default display
		display = gdk_display_open(getenv("DISPLAY"));
		if (display == NULL){
			fprintf(stderr,"No display found.\n");
			return -1;
		}
	}
	
	//open clipboard
	GdkClipboard *clipboard = gdk_display_get_clipboard(display);
	if (clipboard == NULL){
		fprintf(stderr,"couldnt open clipboard\n");
	}

	// set clipbard
	gdk_clipboard_set_text(clipboard,text);
	gdk_clipboard_store_async(clipboard,G_PRIORITY_DEFAULT,NULL,clipboard_set_callback,NULL);
	g_main_loop_run(mainloop);

	//cleanup
	gdk_display_close(display);
	//g_application_run(app);
	g_object_unref(clipboard);
	g_object_unref(app);
	return 0;
}
