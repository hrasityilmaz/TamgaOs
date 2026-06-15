Remove-Item -Force .\TamgaOS.iso -ErrorAction SilentlyContinue

xorriso.exe -as mkisofs `
  -b boot/limine/limine-bios-cd.bin `
  -no-emul-boot `
  -boot-load-size 4 `
  -boot-info-table `
  --protective-msdos-label `
  -o .\TamgaOS.iso `
  .\iso
