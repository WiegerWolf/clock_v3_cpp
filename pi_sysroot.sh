#!/bin/bash

# Accept arguments, default to 'pi' and 'raspberrypi.local' if not provided
REMOTE_USER=${1:-pi}
REMOTE_HOST=${2:-raspberrypi.local}

# Define the local output directory
SYSROOT_DIR="pi_sysroot"

echo "Creating sysroot for ${REMOTE_USER}@${REMOTE_HOST} in directory '${SYSROOT_DIR}'..."

# Create local directories
mkdir -p "${SYSROOT_DIR}/usr"
mkdir -p "${SYSROOT_DIR}/opt"

# Sync /lib
echo "Syncing /lib..."
rsync -avz --rsync-path="rsync" "${REMOTE_USER}@${REMOTE_HOST}:/lib" "${SYSROOT_DIR}/"

# Sync /usr/include
echo "Syncing /usr/include..."
rsync -avz --rsync-path="rsync" "${REMOTE_USER}@${REMOTE_HOST}:/usr/include" "${SYSROOT_DIR}/usr/"

# Sync /usr/lib
echo "Syncing /usr/lib..."
rsync -avz --rsync-path="rsync" "${REMOTE_USER}@${REMOTE_HOST}:/usr/lib" "${SYSROOT_DIR}/usr/"

# (Optional) Sync /opt/vc
echo "Syncing /opt/vc..."
rsync -avz --rsync-path="rsync" "${REMOTE_USER}@${REMOTE_HOST}:/opt/vc" "${SYSROOT_DIR}/opt/"

echo "Done."