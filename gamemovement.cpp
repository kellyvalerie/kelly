#include <ncursesw/ncurses.h>
#include <unistd.h>
#include <cmath>
#include <cstring>

class Heart {
private:
    float x, y;           // Position with floating-point precision for smooth movement
    int lastDrawnX, lastDrawnY; // Last position where the heart was drawn
    float directionX, directionY; // Direction vector (normalized)
    float baseSpeed;      // Base movement speed
    float aspectRatio;    // Character aspect ratio (width/height)
    bool moving;          // Whether the heart is moving
    int symbol;          // Symbol to represent the heart

public:
    Heart(int startX, int startY) : 
        x(static_cast<float>(startX)), y(static_cast<float>(startY)), 
        lastDrawnX(startX), lastDrawnY(startY),
        directionX(0.0f), directionY(0.0f),
        baseSpeed(0.3f), aspectRatio(2.0f), // Assume character cells are about half as wide as they are tall
        moving(false), symbol(ACS_DIAMOND) {}

    void update() {
        if (moving) {
            // Move in the current direction with aspect ratio compensation
            // Horizontal movement is sped up by the aspect ratio
            x += directionX * baseSpeed * aspectRatio;
            // Vertical movement stays at base speed
            y += directionY * baseSpeed;
        }
    }

    void setDirection(float dx, float dy) {
        // Set a new direction vector
        if (dx != 0.0f || dy != 0.0f) {
            // Normalize the direction vector
            float length = sqrt(dx * dx + dy * dy);
            directionX = dx / length;
            directionY = dy / length;
            moving = true;  // Start moving when a direction is set
        }
    }
    
    void setAspectRatio(float ratio) {
        aspectRatio = ratio;
    }
    
    void setSpeed(float speed) {
        baseSpeed = speed;
    }
    
    void stop() {
        moving = false;
    }
    
    void start() {
        moving = true;
    }
    
    bool isMoving() const {
        return moving;
    }

    void setPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }

    void clearPrevious() {
        // Clear the previous position
        mvaddch(lastDrawnY, lastDrawnX, ' ');
    }

    void draw() {
        int currentX = static_cast<int>(round(x));
        int currentY = static_cast<int>(round(y));
        
        // Only redraw if position has changed
        if (currentX != lastDrawnX || currentY != lastDrawnY) {
            // Clear previous position if it's different
            clearPrevious();
            
            // Draw at new position
            attron(COLOR_PAIR(1)); // Red heart color
            mvaddch(currentY, currentX, symbol);
            attroff(COLOR_PAIR(1));
            
            // Update last drawn position
            lastDrawnX = currentX;
            lastDrawnY = currentY;
        } else {
            // Redraw at same position (in case it was overwritten)
            attron(COLOR_PAIR(1)); // Red heart color
            mvaddch(currentY, currentX, symbol);
            attroff(COLOR_PAIR(1));
        }
    }

    float getX() const { return x; }
    float getY() const { return y; }
    float getDirectionX() const { return directionX; }
    float getDirectionY() const { return directionY; }
    float getAspectRatio() const { return aspectRatio; }
    float getSpeed() const { return baseSpeed; }
};

class BattleBox {
private:
    int x, y;         // Top-left corner position
    int width, height; // Box dimensions
    bool needsRedraw;  // Flag to determine if the box needs redrawing

public:
    BattleBox(int startX, int startY, int w, int h) :
        x(startX), y(startY), width(w), height(h), needsRedraw(true) {}

    void draw() {
        if (!needsRedraw) return;
        
        // Enable reverse highlighting
        attron(A_REVERSE);
    
        // Draw the top and bottom borders of the battle box
        for (int i = -1; i <= width+1; i++) {
            mvaddch(y, x + i, ' ');              // Top border (space with reverse highlight)
            mvaddch(y + height, x + i, ' ');     // Bottom border
        }
    
        // Draw the left and right borders of the battle box
        for (int i = 0; i <= height; i++) {
            mvaddch(y + i, x, ' ');              // Left border
            mvaddch(y + i, x + width, ' ');      // Right border
            mvaddch(y + i, x-1, ' ');            // Left border
            mvaddch(y + i, x+1 + width, ' ');    // Right border
        }
    
        // Disable reverse highlighting
        attroff(A_REVERSE);
        
        needsRedraw = false;
    }

    void setNeedsRedraw() {
        needsRedraw = true;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

int main() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);  // Hide cursor
    nodelay(stdscr, TRUE);  // Non-blocking input
    
    // Enable function keys, arrow keys, etc.
    keypad(stdscr, TRUE);
    
    // Set up colors if terminal supports them
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);  // Red heart
    }

    // Get terminal dimensions
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // Create battle box and heart
    BattleBox battleBox(maxX/2 - 20, maxY/2 - 8, 40, 16);
    Heart heart(maxX/2, maxY/2);
    
    // Status line buffer to reduce flickering on text updates
    char statLineMovement[50] = "";
    char statLineDirection[50] = "";
    char statLineAspect[50] = "";
    char statLineSpeed[50] = "";
    char statLinePos[50] = "";
    
    // Draw the static elements once
    battleBox.draw();
    mvprintw(maxY - 3, 2, "Arrow keys to set direction, Space to stop/start");
    mvprintw(maxY - 2, 2, "Q to quit");
    
    // Game loop
    bool running = true;
    while (running) {
        // Process all available input
        int ch;
        bool statusChanged = false;
        
        while ((ch = getch()) != ERR) {
            statusChanged = true;
            if (ch == 'q' || ch == 'Q') {
                running = false;
                break;
            } else if (ch == ' ') {
                // Space toggles movement
                if (heart.isMoving()) {
                    heart.stop();
                } else {
                    heart.start();
                }
            } else if (ch == '+' || ch == '=') {
                // Increase aspect ratio (make horizontal movement faster)
                float aspectRatio = heart.getAspectRatio() + 0.2f;
                if (aspectRatio > 5.0f) aspectRatio = 5.0f;
                heart.setAspectRatio(aspectRatio);
            } else if (ch == '-' || ch == '_') {
                // Decrease aspect ratio (make horizontal movement slower)
                float aspectRatio = heart.getAspectRatio() - 0.2f;
                if (aspectRatio < 1.0f) aspectRatio = 1.0f;
                heart.setAspectRatio(aspectRatio);
            } else if (ch == '[') {
                // Decrease overall speed
                float speed = heart.getSpeed() - 0.05f;
                if (speed < 0.05f) speed = 0.05f;
                heart.setSpeed(speed);
            } else if (ch == ']') {
                // Increase overall speed
                float speed = heart.getSpeed() + 0.05f;
                if (speed > 1.0f) speed = 1.0f;
                heart.setSpeed(speed);
            } else if (ch == KEY_UP) {
                heart.setDirection(0.0f, -1.0f);  // Up
            } else if (ch == KEY_DOWN) {
                heart.setDirection(0.0f, 1.0f);   // Down
            } else if (ch == KEY_LEFT) {
                heart.setDirection(-1.0f, 0.0f);  // Left
            } else if (ch == KEY_RIGHT) {
                heart.setDirection(1.0f, 0.0f);   // Right
            }
        }
        
        // Update heart position
        float oldX = heart.getX();
        float oldY = heart.getY();
        heart.update();
        
        // Boundary checking to keep heart inside the battle box
        float heartX = heart.getX();
        float heartY = heart.getY();
        bool positionConstrained = false;
        
        // Constrain position
        if (heartX < battleBox.getX() + 1) {
            heart.setPosition(static_cast<float>(battleBox.getX() + 1), heartY);
            positionConstrained = true;
        }
        if (heartX > battleBox.getX() + battleBox.getWidth() - 1) {
            heart.setPosition(static_cast<float>(battleBox.getX() + battleBox.getWidth() - 1), heartY);
            positionConstrained = true;
        }
        if (heartY < battleBox.getY() + 1) {
            heart.setPosition(heartX, static_cast<float>(battleBox.getY() + 1));
            positionConstrained = true;
        }
        if (heartY > battleBox.getY() + battleBox.getHeight() - 1) {
            heart.setPosition(heartX, static_cast<float>(battleBox.getY() + battleBox.getHeight() - 1));
            positionConstrained = true;
        }
        
        // Draw the heart
        heart.draw();

        // Refresh screen and control frame rate
        refresh();
        usleep(16667);  // ~60 FPS (1,000,000 microseconds / 60)
    }

    // Clean up
    endwin();
    return 0;
}
