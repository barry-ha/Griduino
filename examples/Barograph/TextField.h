//------------------------------------------------------------------------------
//  File name: TextField.h
//
//  Description: Header file for Adafruit TFT display screens using proportional fonts.
//
//------------------------------------------------------------------------------
//  The MIT License (MIT)
//
//  Copyright (c) 2020 Barry Hansen K7BWH
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//------------------------------------------------------------------------------
#pragma once

#define FLUSHLEFT 0       // align text toward left, using x=left edge of string
#define FLUSHRIGHT 1      // align text toward right, using x=right edge of string

class TextField {
  // Write dynamic text to the TFT display and optimize
  // redrawing text in proportional fonts to reduce flickering
  //
  // Example Usage:
  //      Declare         TextField txtItem("Hello", 64,64, ILI9341_GREEN);
  //      Decl alignment  TextField txtItem("Hello", 64,84, ILI9341_GREEN, ALIGNRIGHT);
  //      Center text     TextField txtItem("Hello", -1,94, ILI9341_GREEN, ALIGNCENTER);
  //      Declare size    TextField txtItem("Hello", 64,84, ILI9341_GREEN, ALIGNLEFT, eFONTSMALL);
  //      Set bkg         txtItem.setBackground(ILI9341_BLACK);
  //      Force one       txtItem.setDirty();
  //      Force all       TextField::setDirty(txtItem, count);
  //      Print one       txtItem.print();
  //
  // To center text left-right, specify x = -1
  //
  // Note about proportional fonts:
  //      1. Text origin is bottom left corner
  //      2. Rect origin is upper left corner
  //      3. Printing text in proportional font does not clear its own background

  private:
    static uint16_t cBackground; // background color
    int16_t xPrev, yPrev;   // remember previous text area for next erasure
    uint16_t wPrev, hPrev;
    char textPrev[27];      // old text to be erased

  public:
    char text[27];          // new text to draw
    int x, y;               // screen coordinates
    uint16_t color;         // text color
    int align;              // FLUSHLEFT | FLUSHRIGHT
    bool dirty;             // true=force reprint even if old=new

    void dump() {
      // dump the state of this object to the console
      char buf[128];
      snprintf(buf, sizeof(buf), "TextField('%s') x,y(%d,%d)", text, x, y);
      Serial.print(buf);
      snprintf(buf, sizeof(buf), " Erase x,y,w,h(%d,%d, %d,%d)", xPrev,yPrev, wPrev, hPrev);
      Serial.println(buf);
    }
    // ctor - dynamic text field
    TextField(int vxx, int vyy, uint16_t vcc, int valign=FLUSHLEFT) {
      init("", vxx, vyy, vcc, valign);
    }
    // ctor - static text field
    TextField(const char vtxt[26], int vxx, int vyy, uint16_t vcc, int valign=FLUSHLEFT) {
      init(vtxt, vxx, vyy, vcc, valign);
    }
    // ctor - text field content specified by a "class String"
    TextField(const String vstr, int vxx, int vyy, uint16_t vcc, int valign=FLUSHLEFT) {
      char temp[ vstr.length()+1 ];
      vstr.toCharArray(temp, sizeof(temp));
      init(temp, vxx, vyy, vcc, valign);
    }
    // common ctor for all data field types
    void init(const char vtxt[26], int vxx, int vyy, uint16_t vcc, int valign) {
      strncpy(textPrev, vtxt, sizeof(textPrev)-1);
      strncpy(text, vtxt, sizeof(text)-1);
      x = vxx;
      y = vyy;
      color = vcc;
      align = valign;
      dirty = true;
      xPrev = yPrev = wPrev = hPrev = 0;
    }

    void print(const char* pText) { // dynamic text
      // main central print routine
      if (dirty || strcmp(textPrev, pText)) {
        eraseOld();
        printNew(pText);
        strncpy(text, pText, sizeof(text));
        strncpy(textPrev, pText, sizeof(textPrev));
        dirty = false;
      }
    }
    void print() {                  // static text
      // delegate to this->print(char*)
      print(text);
    }
    void print(const int d) {       // dynamic integer
      // format integer and delegate to this->print(char*)
      char sInteger[8];
      snprintf(sInteger, sizeof(sInteger), "%d", d);
      print(sInteger);
    }
    void print(const String str) {  // dynamic String
      // format String object and delegate to this->print(char*)
      char temp[ str.length()+1 ];
      str.toCharArray(temp, sizeof(temp));
      print(temp); // delegate to this->print(char*)
    }
    void print(const float f, const int digits) {     // float
      String sFloat = String(f, digits);
      print(sFloat);
    }
    static void setTextDirty(TextField* pTable, int count) {
      // Mark all text fields "dirty" to force reprinting them at next usage
      for (int ii=0; ii<count; ii++) {
        pTable[ii].dirty = true;
      }
    }
    void setBackground(uint16_t bkg) {
      // Set all text field's background color
      cBackground = bkg;
    }

  protected:
    void eraseOld();
    void printNew(const char* pText);
};

