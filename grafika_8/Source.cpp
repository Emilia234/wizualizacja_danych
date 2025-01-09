#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

int main(int argc, char** argv)
{
    ALLEGRO_DISPLAY* display = NULL;
    ALLEGRO_EVENT_QUEUE* event_queue = NULL;

    al_init_primitives_addon();

    if (!al_init()) {
        fprintf(stderr, "failed to initialize allegro!\n");
        return -1;
    }

    display = al_create_display(640, 480);
    if (!display) {
        fprintf(stderr, "failed to create display!\n");
        return -1;
    }

    event_queue = al_create_event_queue();
    if (!event_queue) {
        fprintf(stderr, "failed to create event queue!\n");
        al_destroy_display(display);
        return -1;
    }

    al_register_event_source(event_queue, al_get_display_event_source(display));

    bool running = true;
    float square_x = 50;
    float square_y = 350;
    float square_speed = 2;

    while (running) {
        al_clear_to_color(al_map_rgb(0, 0, 0));

        al_draw_filled_rectangle(200, 50, 300, 120, al_map_rgb(0, 0, 255));
        al_draw_line(350, 50, 450, 150, al_map_rgb(0, 255, 0), 5);
        al_draw_triangle(480, 50, 540, 150, 600, 50, al_map_rgb(255, 255, 0), 5);
        al_draw_filled_rectangle(square_x, square_y, square_x + 100, square_y + 100, al_map_rgb(255, 0, 0));

        square_x += square_speed;

        if (square_x + 100 >= 640 || square_x <= 0) {
            square_speed = -square_speed;
        }

        al_flip_display();
        al_rest(0.016);

  
        ALLEGRO_EVENT event;
        if (al_get_next_event(event_queue, &event)) {
            if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                running = false;
            }
        }
    }

    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}
