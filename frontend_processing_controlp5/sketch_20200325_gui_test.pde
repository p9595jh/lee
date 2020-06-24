import controlP5.*;
import processing.serial.*;
import java.io.*;
import static javax.swing.JOptionPane.*;

ControlP5 cp5;
Serial port;

final String[] labels = { "Set size", "Stretching", "Torsion", "U-shape Bending", "Folding", "Restore" };
final String LOG_PATH = System.getProperty("user.home") + "\\Desktop\\";
BufferedWriter bw;
StringBuilder logBuilder = new StringBuilder();

String csvFileName;
boolean csvWriting = false;

void setup() {
  port = new Serial(this, Serial.list()[0], 9600);
  
  textSize(100);
  size(700, 450);
  background(0);
  //noStroke();
  
  cp5 = new ControlP5(this);
  
  int yPosition = 20;
  final int increaseAs = 70;
  
  cp5.addButton(labels[0])
    .setPosition(50, yPosition)
    .setSize(600, 50)
    .setColorBackground( color(0, 0, 255) )
    .setColorForeground( color(100, 100, 100) )
    .setColorActive( color(50, 50, 50) )
    .setColorLabel( color(255, 255, 255) );
  yPosition += increaseAs;
  
  cp5.addButton(labels[1])
    .setPosition(50, yPosition)
    .setSize(600, 50)
    .setColorBackground( color(0, 0, 255) )
    .setColorForeground( color(100, 100, 100) )
    .setColorActive( color(50, 50, 50) )
    .setColorLabel( color(255, 255, 255) );
  yPosition += increaseAs;
    
  cp5.addButton(labels[2])
    .setPosition(50, yPosition)
    .setSize(600, 50)
    .setColorBackground( color(0, 0, 255) )
    .setColorForeground( color(100, 100, 100) )
    .setColorActive( color(50, 50, 50) )
    .setColorLabel( color(255, 255, 255) );
  yPosition += increaseAs;
    
  cp5.addButton(labels[3])
    .setPosition(50, yPosition)
    .setSize(600, 50)
    .setColorBackground( color(0, 0, 255) )
    .setColorForeground( color(100, 100, 100) )
    .setColorActive( color(50, 50, 50) )
    .setColorLabel( color(255, 255, 255) );
  yPosition += increaseAs;
  
  cp5.addButton(labels[4])
    .setPosition(50, yPosition)
    .setSize(600, 50)
    .setColorBackground( color(0, 0, 255) )
    .setColorForeground( color(100, 100, 100) )
    .setColorActive( color(50, 50, 50) )
    .setColorLabel( color(255, 255, 255) );
  yPosition += increaseAs;
    
  cp5.addButton(labels[5])
    .setPosition(50, yPosition)
    .setSize(600, 50)
    .setColorBackground( color(0, 0, 255) )
    .setColorForeground( color(100, 100, 100) )
    .setColorActive( color(50, 50, 50) )
    .setColorLabel( color(255, 255, 255) );
}

void csvFileWrite(int label) {
  csvFileName = labels[label] + " -" + System.currentTimeMillis() + ".csv";
  csvWriting = true;
}

void controlEvent(ControlEvent e) {
  if ( e.isController() ) {
    
    String name = e.getController().getName();
    if ( name == labels[0] ) {
      int[] arr = new int[2];
      arr[0] = 0;
      try {
        arr[1] = getNumber("Sample size? (mm)");
        port.write( toWritingString(arr) );
      } catch(Exception e1) {
        showMessageDialog(null, "You have to put a number", "ERROR", ERROR_MESSAGE);
      }

    } else if ( name == labels[1] ) {
      int[] arr = new int[4];
      arr[0] = 1;
      try {
        arr[1] = getNumber("Length? (mm)");
        arr[2] = getNumber("Duration? (sec)");
        arr[3] = getNumber("Number of test?");
        port.write( toWritingString(arr) );
        csvFileWrite(1);
      } catch(Exception e1) {
        showMessageDialog(null, "You have to put a number", "ERROR", ERROR_MESSAGE);
      }
      
    } else if ( name == labels[2] ) {
      int[] arr = new int[4];
      arr[0] = 2;
      try {
        arr[1] = getNumber("Angle?");
        arr[2] = getNumber("Duration? (sec)");
        arr[3] = getNumber("Number of test?");
        port.write( toWritingString(arr) );
        csvFileWrite(2);
      } catch(Exception e1) {
        showMessageDialog(null, "You have to put a number", "ERROR", ERROR_MESSAGE);
      }
       
    } else if ( name == labels[3] ) {
      int[] arr = new int[4];
      arr[0] = 3;
      try {
        arr[1] = getNumber("Gap? (mm)");
        arr[2] = getNumber("Duration? (sec)");
        arr[3] = getNumber("Number of test?");
        port.write( toWritingString(arr) );
        csvFileWrite(3);
      } catch(Exception e1) {
        showMessageDialog(null, "You have to put a number", "ERROR", ERROR_MESSAGE);
      }
      
    } else if ( name == labels[4] ) {
      int[] arr = new int[3];
      arr[0] = 4;
      try {
        arr[1] = getNumber("Duration? (sec)");
        arr[2] = getNumber("Number of test?");
        port.write( toWritingString(arr) );
        csvFileWrite(4);
      } catch(Exception e1) {
        showMessageDialog(null, "You have to put a number", "ERROR", ERROR_MESSAGE);
      }
      
    } else if ( name == labels[5] ) {
      try {
        port.write("5");
      } catch(Exception e1) {}
    }
    
  }
}

int getNumber(String content) {
  String s = showInputDialog(null, content, "", PLAIN_MESSAGE);
  return Integer.parseInt(s.trim());
}

String toWritingString(int[] arr) {
  StringBuilder sb = new StringBuilder();
  sb.append(arr[0]);
  sb.append(arr[1]);
  if ( arr.length > 2 ) {
    for (int i=2; i<arr.length; i++) {
      sb.append('|');
      sb.append(arr[i]);
    }
  }
  return sb.toString();
}

void draw() {
  if ( csvWriting ) {
    String s = port.readStringUntil('\n');
    if ( s != null ) {
      String trimmed = s.trim();
      if ( trimmed.equals("00000") ) {
        csvWriting = false;
        try {
          bw = new BufferedWriter(
            new OutputStreamWriter(
              new FileOutputStream(LOG_PATH + csvFileName)));
          bw.write(logBuilder.toString());
          bw.close();
          logBuilder = new StringBuilder();
        } catch(Exception e) {}
      } else {
        logBuilder.append(trimmed);
        logBuilder.append('\n');
      }
    }
  }
}
