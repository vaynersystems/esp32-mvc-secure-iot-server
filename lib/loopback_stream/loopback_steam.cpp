#include "loopback_stream.h"
#include "esp32-hal-psram.h"
loopback_stream::loopback_stream(uint16_t buffer_size) {
  this->buffer = (uint8_t*) ps_malloc(buffer_size);
  this->buffer_size = buffer_size;
  this->pos = 0;
  this->size = 0;
}
loopback_stream::~loopback_stream() {
  free(buffer);
}

void loopback_stream::clear() {
  this->pos = 0;
  this->size = 0;
}

int loopback_stream::read() {
  if (size == 0) {
    return -1;
  } else {
    int ret = buffer[pos];
    pos++;
    size--;
    if (pos == buffer_size) {
      pos = 0;
    }
    return ret;
  }
}

size_t loopback_stream::write(uint8_t v) {
  if (size == buffer_size) {
    return 0;
  } else {
    int p = pos + size;
    if (p >= buffer_size) {
      p -= buffer_size;
    }
    buffer[p] = v;
    size++;
    return 1;
  }  
}

int loopback_stream::available() {
  return size;
}

int loopback_stream::availableForWrite() {
  return buffer_size - size;
}

bool loopback_stream::contains(char ch) {
  for (int i=0; i<size; i++){
    int p = (pos + i) % buffer_size;
    if (buffer[p] == ch) {
      return true;
    }
  }
  return false;
}

int loopback_stream::peek() {
  return size == 0 ? -1 : buffer[pos];
}

void loopback_stream::flush() {
  //I'm not sure what to do here...
}

