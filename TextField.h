#ifndef _GRIDUINO_TEXTFIELD_H
#define _GRIDUINO_TEXTFIELD_H

#define FLUSHLEFT 0       // align text toward left, using x=left edge of string
#define FLUSHRIGHT 1      // align text toward right, using x=right edge of string

class TextField {
  // Write dynamic text to the TFT display and optimize
  // redrawing text in proportional fonts to reduce flickering
  //
  // Example Usage:
  //      Declare     TextField txtItem("Hello", 64,64, ILI9341_GREEN);
  //      Set bkg     txtItem.setBackground(ILI9341_BLACK);
  //      Force       txtItem.setDirty();
  //      Print       txtItem.print();
  //
  // Note about proportional fonts:
  // 1. Text origin is bottom left corner
  // 2. Rect origin is upper left corner
  // 2. Printing text does not clear its own background

  private:
    static uint16_t cBackground; // background color

  public:
    char prevText[27];      // old text to be erased
    char text[27];          // new text to draw
    int x, y;               // screen coordinates
    uint16_t color;         // text color
    int align;              // FLUSHLEFT | FLUSHRIGHT
    bool dirty;             // true=force reprint even if old=new

    // ctor - dynamic text field
    TextField(int vxx, int vyy, uint16_t vcc, int valign=FLUSHLEFT) {
      init("", vxx, vyy, vcc, valign);
    }
    // ctor - static text field
    TextField(const char vtxt[26], int vxx, int vyy, uint16_t vcc, int valign=FLUSHLEFT) {
      init(vtxt, vxx, vyy, vcc, valign);
    }
    // ctor - static String field
    TextField(const String vstr, int vxx, int vyy, uint16_t vcc, int valign=FLUSHLEFT) {
      char temp[ vstr.length()+1 ];
      vstr.toCharArray(temp, sizeof(temp));
      init(temp, vxx, vyy, vcc, valign);
    }
    // delegating ctor for common setup code
    void init(const char vtxt[26], int vxx, int vyy, uint16_t vcc, int valign) {
      strncpy(prevText, vtxt, sizeof(prevText)-1);
      strncpy(text, vtxt, sizeof(text)-1);
      x = vxx;
      y = vyy;
      color = vcc;
      align = valign;
      dirty = true;
    }

    void print() {                  // static text
      printNew(prevText);
      dirty = false;
     }
    void print(const int d) {       // dynamic integer
      char sInteger[8];
      snprintf(sInteger, sizeof(sInteger), "%d", d);
      print(sInteger);
    }
    void print(const char* pText) { // dynamic text
      if (dirty || strcmp(prevText, pText)) {
        eraseOld();
        printNew(pText);
        strncpy(prevText, pText, sizeof(prevText));
        dirty = false;
      }
    }
    void print(const String str) {  // dynamic String
      char temp[ str.length()+1 ];
      str.toCharArray(temp, sizeof(temp));
      print(temp);
    }
    static void setTextDirty(TextField* pTable, int count) {
      // Mark all text fields "dirty" to force reprinting them at next usage
      for (int ii=0; ii<count; ii++) {
        pTable[ii].dirty = true;
      }
    }
    void setBackground(uint16_t bkg) {
      cBackground = bkg;
    }

  private:
    void eraseOld();
    void printNew(const char* pText);
};

#endif // _GRIDUINO_TEXTFIELD_H
