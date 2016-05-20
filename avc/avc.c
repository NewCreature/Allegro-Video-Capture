#include <allegro5/allegro5.h>

/* Runs the mencoder command to encode our video frames and audio into the
   final output video file. */
static bool avc_encode_video(const char * fn, int fps)
{
    char sys_command[1024];
    ALLEGRO_PATH * path;

    path = al_create_path(fn);
    if(!path)
    {
        return false;
    }
    al_set_path_filename(path, "");
    snprintf(sys_command, 1024, "mencoder %s/img_*.png -oac mp3lame -ovc x264 -fps %d -vf harddup -audiofile %s/audio.wav -o %s", al_path_cstr(path, '/'), fps, al_path_cstr(path, '/'), fn);

    return true;
}

/* Run logic_proc() until it returns false, rendering each frame with
   rener_proc() and saving the output to sequentially named image files. */
static bool avc_capture_video(
    ALLEGRO_DISPLAY * dp,
    const char * fn,
    bool (*logic_proc)(void * data),
    void (*render_proc)(void * data),
    int fps,
    int flags,
    void * data)
{
    ALLEGRO_STATE old_state;
    ALLEGRO_BITMAP * buffer;
    char out_fn[1024];
    int frame = 0;

    al_store_state(&old_state, ALLEGRO_STATE_TARGET_BITMAP);

    /* capture images first */
    buffer = al_create_bitmap(al_get_display_width(dp), al_get_display_height(dp));
    if(!buffer)
    {
        return false;
    }
    al_set_target_bitmap(buffer);
    while(logic_proc(data))
    {
        render_proc(data);
        snprintf(out_fn, 1024, "img_%7d.png", frame);
        al_save_bitmap(out_fn, buffer);
        frame++;
    }
    al_restore_state(&old_state);
    al_destroy_bitmap(buffer);

    return true;
}

/* Run logic_proc() at fps frequency until it returns false, capturing audio to
   a WAV file. */
static bool avc_capture_audio(
    const char * fn,
    bool (*logic_proc)(void * data),
    int fps,
    int flags,
    void * data)
{
    ALLEGRO_EVENT_QUEUE * event_queue = NULL;
    ALLEGRO_TIMER * logic_timer = NULL;
    ALLEGRO_EVENT event;

    event_queue = al_create_event_queue();
    if(!event_queue)
    {
        return false;
    }

    logic_timer = al_create_timer((float)fps / 60.0);
    if(!logic_timer)
    {
        return false;
    }

    al_register_event_source(event_queue, al_get_timer_event_source(logic_timer));
    al_start_timer(logic_timer);
    while(1)
    {
        al_wait_for_event(event_queue, &event);
		if(!logic_proc(data))
        {
            break;
        }
    }
    al_destroy_timer(logic_timer);
    al_destroy_event_queue(event_queue);
    return true;
}

/* Capture audio and video produced by logic_proc() and render_proc() to a
   video file named out_filename. init_proc() is called at the start of each
   pass to reset the state of the program. */
bool avc_start_capture(ALLEGRO_DISPLAY * display,
    const char * out_filename,
    bool (*init_proc)(void * data),
    bool (*logic_proc)(void * data),
    void (*render_proc)(void * data),
    int fps,
    int flags,
    void * data)
{
    /* capture video */
    if(!init_proc(data))
    {
        return false;
    }
    if(!avc_capture_video(display, out_filename, logic_proc, render_proc, fps, flags, data))
    {
        return false;
    }

    /* capture sound */
    if(!init_proc(data))
    {
        return false;
    }
    if(!avc_capture_audio(out_filename, logic_proc, fps, flags, data))
    {
        return false;
    }

    /* encode video */
    avc_encode_video(out_filename, fps);

    return true;
}