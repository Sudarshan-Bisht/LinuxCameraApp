#include <gtk/gtk.h>
#include <gst/gst.h>
#include <glib.h>

// GTK
GtkWidget *startsnapbutton;
GtkWidget *snapbutton;
GtkWidget *stopsnapbutton;

// Gstreamer
static GstElement *camPipeline;
GstElement *cambin;
GstBus *cambus;

//GLIB
GMainLoop *camloop;
guint cam_bus_watch_id;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *)data;
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

        case GST_MESSAGE_ERROR:
        {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }

        case GST_MESSAGE_APPLICATION:
        {
            const GstStructure *s;
            s = gst_message_get_structure(msg);

            if (gst_structure_has_name(s, "GstLaunchInterrupt"))
            {
                /* this application message is posted when we caught an interrupt and
                * we need to stop the pipeline. */
                g_print("Interrupt: Stopping pipeline ...\n");
                /* gst_element_send_event (camPipeline, gst_event_new_eos ()); */
                gst_element_set_state(camPipeline, GST_STATE_NULL);
                g_main_loop_quit(loop);
            }
            break;
        }
        
        default:
            break;
    }
    return TRUE;
}

gint delete_event(GtkWidget *widget, GdkEvent  *event, gpointer   data)
{
    g_print("Delete_Event called\n");
    return(FALSE);
}

/* Start the Camera - Set up camerabin.  */
void snapButtonPressed(GtkWidget *widget, gpointer data)
{
    GstCaps *caps;
    
    gtk_widget_set_sensitive(startsnapbutton, FALSE);
    gtk_widget_set_sensitive(snapbutton, TRUE);
    gtk_widget_set_sensitive(stopsnapbutton, TRUE);
 
    camPipeline = gst_pipeline_new("camera");
    cambin = gst_element_factory_make("camerabin", "cambin1");

    camloop = g_main_loop_new(NULL, FALSE);
    if (!camPipeline || !cambin )
    {
        g_printerr ("One element could not be created. Exiting.\n");
        return;
    }
    
    /* Set up the pipeline */
    g_print ("Watch for the bus\n");
    cambus = gst_pipeline_get_bus(GST_PIPELINE (camPipeline));
    cam_bus_watch_id =  gst_bus_add_watch(cambus, bus_call, camloop);

    g_print("Set the picture size\n");
    caps = gst_caps_from_string("video/x-raw, width=(int)640, height=(int)360");
    g_object_set(G_OBJECT(cambin), "image-capture-caps", caps, NULL);
    gst_caps_unref(caps); /*  This unref seems to cause problems and may need to be commented out */

    g_print ("Add the element to the pipeline\n");
    gst_bin_add_many(GST_BIN(camPipeline), cambin, NULL);
    
    gst_element_set_state(camPipeline, GST_STATE_PLAYING);

    /* Iterate */
    g_main_loop_run(camloop);
    
    /* Out of the main loop, clean up nicely */
    g_print ("Returned, Camera Off\n");
    gst_element_set_state(camPipeline, GST_STATE_NULL);

    gst_object_unref(GST_OBJECT(camPipeline));
    g_source_remove(cam_bus_watch_id);
    g_main_loop_unref(camloop);
 
    gtk_main_quit();

    return;
}

void snapNow(GtkWidget *widget, gpointer data)
{
    guint lbIdle;
 
    g_print("Take a picture\n");
 
    gchar* filename;
    GTimeVal time;
    g_get_current_time(&time);
    filename = g_time_val_to_iso8601(&time);
    g_object_set(G_OBJECT(cambin), "location", filename, NULL);

    g_object_get(cambin, "idle", &lbIdle, NULL);
    if (lbIdle)
    {
        g_print("Camera is Idle\n");
    }
    else
    {
        g_print("Camera is Busy\n");
    }
 
    g_signal_emit_by_name(cambin, "start-capture", NULL);
}


void snapOff(GtkWidget *widget, gpointer data)
{
    g_print ("We want to stop\n");

    gst_element_post_message(GST_ELEMENT(camPipeline),
        gst_message_new_application(GST_OBJECT(camPipeline),
            gst_structure_new("GstLaunchInterrupt",
                "message", G_TYPE_STRING, "Pipeline interrupted", NULL)));    
}


int main(int argc, char *argv[])
{
    /* GtkWidget is the storage type for widgets */
    GtkWidget *window;
    GtkWidget *grid;
   
    /* This is called in all GTK applications. Arguments are parsed
     * from the command line and are returned to the application. */
    gtk_init(&argc, &argv);
    gst_init(&argc, &argv);

    /* Create a new window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window,50,50);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_title(GTK_WINDOW (window), "Camera");
    
    /* Here we just set a handler for delete_event that immediately exits GTK. */
    g_signal_connect(window, "destroy", G_CALLBACK (delete_event), NULL);

    /* Sets the border width of the window. */
    gtk_container_set_border_width(GTK_CONTAINER (window), 10);

    /* Use a grid for the buttons */
    grid = gtk_grid_new();

    /* Put the grid into the main window. */
    gtk_container_add(GTK_CONTAINER (window), grid);
   
     /* Snapshot Buttons. */
    startsnapbutton = gtk_button_new_with_label("Start Camera");
    g_signal_connect(startsnapbutton, "clicked", G_CALLBACK (snapButtonPressed), "Snapshot");
    gtk_grid_attach(GTK_GRID(grid), startsnapbutton, 0, 1, 1, 1);
    gtk_widget_show(startsnapbutton);

    snapbutton = gtk_button_new_with_label("Take Photo");
    g_signal_connect(snapbutton, "clicked", G_CALLBACK (snapNow), "Snapshot");
    gtk_grid_attach(GTK_GRID(grid), snapbutton, 0, 2, 1, 1);
    gtk_widget_show(snapbutton);
    gtk_widget_set_sensitive(snapbutton, FALSE);


    stopsnapbutton = gtk_button_new_with_label("Stop Camera");
    g_signal_connect(stopsnapbutton, "clicked", G_CALLBACK (snapOff), "Snapshot");
    gtk_grid_attach(GTK_GRID(grid), stopsnapbutton, 0, 3, 1, 1);
    gtk_widget_show(stopsnapbutton);
    gtk_widget_set_sensitive(stopsnapbutton, FALSE);
        
    gtk_widget_show(grid);

    gtk_widget_show(window);

    /* Rest in gtk_main and wait for the fun to begin! */
    gtk_main();

    return(0);
}

