#pragma once

#include <Stream.h>

/*
 * A LoopbackStream stores all data written in an internal buffer and returns it back when the stream is read.
 * 
 * If the buffer overflows, the last bytes written are lost.
 * 
 * It can be used as a buffering layer between components.
 */
class loopback_stream : public Stream {
  uint8_t *buffer;
  uint16_t buffer_size;
  uint16_t pos, size;
public:
  static const uint16_t DEFAULT_SIZE = 64;
  
  loopback_stream(uint16_t buffer_size = loopback_stream::DEFAULT_SIZE);
  ~loopback_stream();
    
  /** Clear the buffer */
  void clear(); 
  
  virtual size_t write(uint8_t);
  virtual int availableForWrite(void);
  
  virtual int available();
  virtual bool contains(char);
  virtual int read();
  virtual int peek();
  virtual void flush();
};
