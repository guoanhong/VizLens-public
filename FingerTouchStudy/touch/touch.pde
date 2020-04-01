import processing.sound.*;

boolean enableSound = true;
boolean enableFlash = true;

int counter;

boolean touching;

SoundFile file;

TouchEvent touchEvent;

void setup() {
  //touchpoints = 0;
  counter = 0;
  touching = false;
  
  // Load a soundfile from the /data folder of the sketch and play it back
  file = new SoundFile(this, "ding.mp3");
  
  size(displayWidth, displayHeight);
  noStroke();
  textAlign(CENTER);
  ellipseMode(CENTER);
  background(0);
}

void draw() {
  background(0);
  /*
  if (mousePressed) {
    if (enableFlash) {
      background(255);
    } 
    fill(0, 255, 0);
    textSize(100);
    text("Clicked", width / 2, height / 2);
    noStroke();
    fill(180, 180, 100);
    touchpoints = touchEvent.touches.length;
    for (int i = 0; i < touchpoints; i++) {
      int x = touchEvent.touches[i].offsetX;
      int y = touchEvent.touches[i].offsetY;
      ellipse(x, y, 150, 150);
    }
  }
  */
  
  if (mousePressed) {
    fill(180, 180, 100);
    ellipse(mouseX, mouseY, 600, 600);
  }
  
  fill(255);
  textSize(450);
  //text(touchpoints, 300, 300);
  text(counter, width - 300, 450);
}

void mousePressed() {
  if (enableSound) {
    file.play();
  }
  counter++;
  //touchpoints = 1;
}

void mouseReleased() {
}

void mouseMoved()
{
   //can do stuff everytime the mouse is moved (i.e., not clicked)
   //https://processing.org/reference/mouseMoved_.html
}

void mouseDragged()
{
  //can do stuff everytime the mouse is dragged
  //https://processing.org/reference/mouseDragged_.html
}
