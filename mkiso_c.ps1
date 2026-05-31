Remove-Item -Force .\TamgaOS_C.iso -ErrorAction SilentlyContinue

xorriso.exe -as mkisofs `
  -b boot/limine/limine-bios-cd.bin `
  -no-emul-boot `
  -boot-load-size 4 `
  -boot-info-table `
  --protective-msdos-label `
  -o .\TamgaOS_C.iso `
  .\iso_c
