/* ONLY USE THIS FILE WHEN COMPILING FOR EMSCRIPTEN!!!!! */

#undef main

int
main (int argc, char *argv[])
{
  /* The assumption here is that libhdf5.settings does not exist on
     the emscripten run-time file system, so we first fill it from
     stdin. */
  FILE *settings = fopen ("libhdf5.settings", "wt");
  /* Copy what we get from stdin to settings */
  size_t buffer_size = 4096;
  char *buffer = malloc (buffer_size);
  if (buffer_size == NULL)
    {
      abort ();
    }
  size_t n_read;
  while ((n_read = fread (buffer, 1, buffer_size, stdin)) != 0)
    {
      fwrite (buffer, 1, n_read, settings);
    }
  fclose (settings);
  return true_main(argc, argv);
}
