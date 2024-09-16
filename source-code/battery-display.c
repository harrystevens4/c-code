#include <stdio.h>
#include <gtk/gtk.h>
#include <unistd.h>
int get_battery_percent(){
	if (access("/sys/class/power_supply/BAT0/charge_now",F_OK) != 0){
		fprintf(stderr,"ERROR: could not open the battery level file.\n");
		return -1;
	}
	
	FILE *charge_now_fd = fopen("/sys/class/power_supply/BAT0/charge_now","r");
	FILE *charge_full_fd = fopen("/sys/class/power_supply/BAT0/charge_full","r");
	if (charge_now_fd == NULL || charge_full_fd == NULL){
		fprintf(stderr,"ERROR: Could not open battery charge file descriptors\n");
		perror("fopen");
		return -1;
	}
	
	char buffer[1024];
	int buffer_size = fread(buffer,1,1024,charge_now_fd);
	if (buffer_size == 0){
		fprintf(stderr,"ERROR: Could not read file.\n");
	}
	int charge_now = strtol(buffer,NULL,10);
	
	buffer_size = fread(buffer,1,1024,charge_full_fd);
	if (buffer_size == 0){
		fprintf(stderr,"ERROR: Could not read file.\n");
	}
	int charge_full = strtol(buffer,NULL,10);

	double percentage = ((double)charge_now/(double)charge_full);
	percentage = percentage*100;
	fclose(charge_now_fd);
	fclose(charge_full_fd);
	return percentage;
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
	char label_string[1024];
	snprintf(label_string,1024,"Battery level: %d%%",get_battery_percent());
	GtkWidget *label = gtk_label_new(label_string);
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
