#include <stdio.h>
#include <gtk/gtk.h>
int get_batter_level(){
}
int activate(GtkApplication *app,gpointer user_data){
	
	//set up window
	GtkWidget *window;
	window = gtk_application_window_new(app);
	GtkWidget *box;
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

	//make widgets
	GtkWidget *label = gtk_label_new("Battery level: ");
	GtkWidget *battery_level_bar_widget = gtk_level_bar_new();
	GtkLevelBar *batter_level_bar = GTK_LEVEL_BAR(battery_level_bar_widget);

	//display widgets
	gtk_box_append(GTK_BOX(box), label);
	gtk_box_append(GTK_BOX(box), battery_level_bar_widget);

	//present window
	gtk_window_present(GTK_WINDOW(window));
}
int main(int argc, char **argv){
	GtkApplication *app;
	app = gtk_application_new("com.github.rufus173.BatteryDisplay",G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);
	int status = g_application_run(G_APPLICATION(app),argc,argv);
	g_object_unref(app);
	return status;
}
