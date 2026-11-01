# Lanet OS: quick start

The project builds a lightweight Alpine Linux 3.24 x86_64 live ISO using Alpine's official `mkimage.sh` tooling.

## Build on Windows with an existing Alpine VM

A VM named **Lanet OS Builder Alpine** is already created and stopped. It has 1536 MB RAM, 2 CPUs, and a 20 GB virtual disk. The ISO `output/alpine-standard-3.24.3-x86_64.iso` is attached as the DVD.

1. Start the VM from VirtualBox or run `VBoxManage startvm "Lanet OS Builder Alpine"` in Windows PowerShell.
2. Install Alpine inside the VM and make sure networking works. This computer has only 4 GB RAM, so do not raise the VM memory above 1536 MB.
3. Copy this project directory into the Alpine VM. Install the build tools:

   ```sh
   doas apk add git
   ```

4. From the copied project directory, run:

   ```sh
   doas sh ./build-lanetos.sh
   ```

5. The generated ISO is `output/lanetos-alpine-3.24-x86_64.iso`. Attach it to a separate test VM and boot it before trying a physical computer.

The first build needs internet and about 20 GB of free space. Expect roughly 30-90 minutes; this is an estimate, not a guarantee. Keep the builder VM and test VM separate so the generated image is not accidentally installed over the build environment.

## First boot

The updated build is intended to start the Lanet user session automatically: OpenRC starts greetd, greetd launches Sway, and Sway launches Lanet Shell. A first-boot service installs the Lanet APK and configures eudev/input devices. These changes require a newly built ISO; an older ISO will not have them.

Test in a separate VM first. The live desktop uses RAM, so this minimal image is a better fit for low-memory computers than a large desktop, but actual hardware requirements still need testing. Do not select a system disk in `setup-alpine` unless you intend to erase that disk.

## Customize

Change the Alpine image profile in `mkimg.lanetos.sh`. Rebuild, boot the ISO in a VM, and confirm the changes before using it on a real PC. See [README.md](README.md) for scope and known limitations.
