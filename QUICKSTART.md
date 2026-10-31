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

This is an Alpine live/installer image, not a finished graphical desktop. At the console, run `setup-alpine` to configure/install the base system. Follow the prompts carefully; selecting a disk for system installation erases it. After rebooting into the installed system, run `setup-desktop xfce` to configure XFCE, or choose a lighter desktop/window manager for a weaker PC.

For very old or low-memory PCs, use a disk installation rather than expecting a full graphical desktop to run from the live ISO. The base system is small, but XFCE and a browser need more RAM. Test with your target hardware before making performance claims.

## Customize

Change the Alpine image profile in `mkimg.lanetos.sh`. Rebuild, boot the ISO in a VM, and confirm the changes before using it on a real PC. See [README.md](README.md) for scope and known limitations.
