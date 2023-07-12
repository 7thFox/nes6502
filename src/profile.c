#include "headers/profile.h"

unsigned long _profilerStart;
FILE   *_profilerEvents = NULL;
char   *_profilerEventsB;
size_t  _profilerEventsS = 0;
FILE   *_profilerFrames = NULL;
char   *_profilerFramesB;
size_t  _profilerFramesS = 0;
void **_profileFrameFns;
size_t _profileFrameFnsS;
size_t _profilerNextFrame;


#define PROFILE_MIN 0 // ~80% of un-minimized

long get_frame(void *fn) {
    if (_profileFrameFns) {

        for (size_t x = 0; x < _profilerNextFrame; x++) {
            if (_profileFrameFns[x] == fn) return x;
        }

        if (_profilerNextFrame < _profileFrameFnsS)
        {
            size_t i = _profilerNextFrame;
            _profilerNextFrame++;
            _profileFrameFns[i] = fn;


            char **   symbols = backtrace_symbols(&fn, 1);
#if PROFILE_MIN
            fprintf(_profilerFrames, "{\"name\":\"%s\"},", symbols[0]);
#else
            fprintf(_profilerFrames, "      {\"name\":\"%s\"},\n", symbols[0]);
#endif

            free(symbols);
            return i;
        }
    }
    return -1;
}

void init_profiler() {
    _profilerEvents = open_memstream(&_profilerEventsB, &_profilerEventsS);
    _profilerFrames = open_memstream(&_profilerFramesB, &_profilerFramesS);

    _profilerNextFrame = 0;
    _profileFrameFnsS = 512;
    _profileFrameFns = malloc(sizeof(void*) * _profileFrameFnsS);
    if (!_profileFrameFns) _profileFrameFnsS = 0;

    unsigned int ui;
    _profilerStart = __rdtscp(&ui);
}

void end_profiler(const char *file_name) {
    if (_profilerEvents)
    {
        fflush(_profilerEvents);
        fclose(_profilerEvents);
        _profilerEvents = NULL;
    }
    if (_profilerFrames)
    {
        fflush(_profilerFrames);
        fclose(_profilerFrames);
        _profilerFrames = NULL;
    }

    if (_profilerEventsB)// && _profilerFramesB)
    {
        unsigned long profilerEnd;
        unsigned int ui;
        profilerEnd = __rdtscp(&ui);

        FILE *fd = fopen(file_name, "w");
        if (fd)
        {
#if PROFILE_MIN
            fprintf(fd, "{");
            fprintf(fd, "\"version\":\"0.0.1\",");
            fprintf(fd, "\"$schema\":\"https://www.speedscope.app/file-format-schema.json\",");
            fprintf(fd, "\"shared\":{");
            fprintf(fd, "\"frames\":[");
            fwrite(_profilerFramesB, sizeof(char), _profilerFramesS-2, fd);// -2 for comma+newline
            fprintf(fd, "]");
            fprintf(fd, "},");
            fprintf(fd, "\"profiles\":[");
            fprintf(fd, "{");
            fprintf(fd, "\"type\":\"evented\",");
            fprintf(fd, "\"name\":\"simple.txt\",");
            fprintf(fd, "\"unit\":\"none\",");
            fprintf(fd, "\"startValue\":%lu,", _profilerStart);
            fprintf(fd, "\"endValue\":%lu,", profilerEnd);
            fprintf(fd, "\"events\":[");
            fwrite(_profilerEventsB, sizeof(char), _profilerEventsS, fd);
            fprintf(fd, "}");
            fprintf(fd, "]");
            fprintf(fd, "}");
#else
            fprintf(fd, "{\n");
            fprintf(fd, "  \"version\": \"0.0.1\",\n");
            fprintf(fd, "  \"$schema\": \"https://www.speedscope.app/file-format-schema.json\",\n");
            fprintf(fd, "  \"shared\": {\n");
            fprintf(fd, "    \"frames\": [\n");
            fwrite(_profilerFramesB, sizeof(char), _profilerFramesS-2, fd);// -2 for comma+newline
            fprintf(fd, "\n");
            fprintf(fd, "    ]\n");
            fprintf(fd, "  },\n");
            fprintf(fd, "  \"profiles\": [\n");
            fprintf(fd, "    {\n");
            fprintf(fd, "      \"type\": \"evented\",\n");
            fprintf(fd, "      \"name\": \"simple.txt\",\n");
            fprintf(fd, "      \"unit\": \"none\",\n");
            fprintf(fd, "      \"startValue\": %lu,\n", _profilerStart);
            fprintf(fd, "      \"endValue\": %lu,\n", profilerEnd);
            fprintf(fd, "      \"events\": [\n");
            fwrite(_profilerEventsB, sizeof(char), _profilerEventsS-2, fd);// -2 for comma+newline
            fprintf(fd, "\n");
            fprintf(fd, "      ]\n");
            fprintf(fd, "    }\n");
            fprintf(fd, "  ]\n");
            fprintf(fd, "}");
#endif

            fflush(fd);
            fclose(fd);
        }
    }
    if (_profilerEventsB) free(_profilerEventsB);
    if (_profilerFramesB) free(_profilerFramesB);
}

void __cyg_profile_func_enter (void *func __attribute__((unused)), void *caller  __attribute__((unused)))
{
    if (!_profilerEvents) return;

    long fi = get_frame(func);

    unsigned long i;
    unsigned int ui;
    i = __rdtscp(&ui);
#if PROFILE_MIN
    fprintf(_profilerEvents, "{\"type\":\"O\",\"frame\":%lu,\"at\":%lu},", fi, i);
#else
    fprintf(_profilerEvents, "        {\"type\":\"O\",\"frame\":%lu,\"at\":%lu},\n", fi, i);
#endif
}


void __cyg_profile_func_exit (void *func  __attribute__((unused)), void *caller  __attribute__((unused)))
{
    if (!_profilerEvents) return;

    unsigned long i;
    unsigned int ui;
    i = __rdtscp(&ui);

    long fi = get_frame(func);

#if PROFILE_MIN
    fprintf(_profilerEvents, "{\"type\":\"C\",\"frame\":%lu,\"at\":%lu},", fi, i);
#else
    fprintf(_profilerEvents, "        {\"type\":\"C\",\"frame\":%lu,\"at\":%lu},\n", fi, i);
#endif
}
