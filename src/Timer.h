#pragma once

class FunctionTimer
{
  private:
    char* Name = nullptr;
    unsigned long Start = 0;
  public:
    FunctionTimer( char* name )
    {
      Name = name;
      Start = millis();
    }
    ~FunctionTimer()
    {
      log_v("Timer(%s): %d", Name, millis() - Start);
    }
};

class FunctionTimer_Group
{
  private:
    char* Name = nullptr;
    unsigned long Total = 0;
    unsigned long Calls = 0;
  public:
    FunctionTimer_Group( char* name )
    {
      Name = name;
    }
    ~FunctionTimer_Group()
    {
      log_v("Timer(%s): %d total in %d calls", Name, Total, Calls);
    }
    void Add( unsigned long timer )
    {
      Total += timer;
      Calls++;
    }
};

class FunctionTimer_Sub
{
  private:
    FunctionTimer_Group* Group = nullptr;
    unsigned long Start = 0;
  public:
    FunctionTimer_Sub( FunctionTimer_Group* group )
    {
      Group = group;
      Start = millis();
    }
    ~FunctionTimer_Sub()
    {
      if( Group )
        Group->Add(millis()-Start);
    }
};
