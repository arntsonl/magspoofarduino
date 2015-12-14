/*
 * Arduino MagSpoof
 *   Ported by Luke Arntson
 *   Original by Samy Kamkar
 * 
 * http://github.com/arntsonl/magspoofarduino/
 * http://samy.pl/magspoof/
 */

int CLOCK_US = 330;

int sw1Pin = 12;
int enablePin = 11;
int in1Pin = 10;
int in2Pin = 9;

int dir = 0;

// up to 128 bytes per track
const char* tracks[] = {
"%B123456781234567^LASTNAME/FIRST^YYMMSSSDDDDDDDDDDDDDDDDDDDDDDDDD?\0", // Track 1
";123456781234567=YYMMSSSDDDDDDDDDDDDDD?\0" // Track 2
};

char outTrack1[67];
char outTrack2[41];

const int sublen[] = {
  32, 48, 48
};

const int bitlen[] = {
  7, 5, 5
};

void setup() {
  // put your setup code here, to run once:
  pinMode(sw1Pin, INPUT);
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  Serial.begin(115200);
}

void playBit(int sendBit)
{
  dir ^= 1;
  digitalWrite(in1Pin, dir);
  digitalWrite(in2Pin, !dir);
  delayMicroseconds(CLOCK_US);
  Serial.print(dir);

  if(sendBit)
  {
    dir ^= 1;
    digitalWrite(in1Pin, dir);
    digitalWrite(in2Pin, !dir);
    Serial.print(dir);
  }
  delayMicroseconds(CLOCK_US);
}

void playTrack(char * data, int trackNum)
{
  int i, j;
  dir = 0;
  for(i = 0; data[i] != '\0'; i++)
  {
    for (j = 0; j < bitlen[trackNum]-1; j++)
    {
      playBit((data[i] >> j) & 1);
    }
  }
}

void playTrackReverse(char * data, int trackNum)
{
  int i=0, j;
  dir = 0;
  while(data[i++] != '\0');
  i--;
  while(i--)
  {
    for (j = bitlen[trackNum]-1; j >= 0; j--)
    {
      playBit((data[i] >> j) & 1);
    }
  }
}

void generateTrack(const char * data, char * out, int trackNum)
{
  int i, j, tmp, crc, lrc = 0;
  dir = 0;

  for(i = 0; data[i] != '\0'; i++)
  {
    crc = 1;
    tmp = data[i] - sublen[trackNum];
    for (j = 0; j < bitlen[trackNum]-1; j++)
    {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      tmp & 1 ? (out[i] |= (1<<j)) : (out[i] &= ~(1 << j));
      tmp >>= 1;
    }
    crc ? (out[i] |= (1 << 4)) : (out[i] &= ~(1 << 4));
  }

  // finish calculating and send last "byte" (LRC)
  tmp = lrc;
  crc = 1;
  for(j = 0; j < bitlen[trackNum]-1; j++)
  {
    crc ^= tmp & 1;
    tmp & 1 ? (out[i] |= (1 << j)) : (out[i] &= ~(1 << j));
    tmp >>= 1;
  }
  crc ? (out[i] |= (1 << 4)) : (out[i] &= ~(1 << 4));
  i++;
  out[i] = '\0';
}

void loop() {
  int i;
  char character;
  int reading;

  // Wait for digital input from switch 1
  reading = digitalRead(sw1Pin);
  if ( reading == 0 )
  {
    return;
  }

  // Generate our tracks
  generateTrack(tracks[0], outTrack1, 0);
  generateTrack(tracks[1], outTrack2, 0);

  noInterrupts();

  // enable H-Bridge and LED
  digitalWrite(enablePin, HIGH);

  // Play 25 0s
  for(i = 0; i < 25; i++)
    playBit(0);
  
  // play back track 1
  playTrack(outTrack1, 0);

  // 53 zeros between track1 & 2
  for(i = 0; i < 53; i++)
      playBit(0);
  
  // play back track 2
  playTrackReverse(outTrack2, 1);

  // Play 25 0s
  for(i = 0; i < 25; i++)
    playBit(0);

  // disable H-Bridge and LED
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
  digitalWrite(enablePin, LOW);

  delay(400); // let the chip cool down
  interrupts();
}

