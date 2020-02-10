#!/usr/bin/env node

const path = require('path');
const fs = require('fs-extra');

const configureIOS = require('./configureIOS');
const configureAndroid = require('./configureAndroid');
const { findProjectRootDirAsync } = require('./tools');

const ERROR_FILE_PATH = path.join(__dirname, './error.log'); // eslint-disable-line

async function main() {
  try {
    const packageDir = path.dirname(__dirname); // eslint-disable-line
    const rootDir = await findProjectRootDirAsync(path.dirname(packageDir));

    await configureAndroid(rootDir);
    await configureIOS(rootDir);

    if (await fs.pathExists(ERROR_FILE_PATH)) {
      await fs.remove(ERROR_FILE_PATH);
    }
    process.exit(0);
  } catch (error) {
    await fs.outputFile(ERROR_FILE_PATH, error.stack);
  }

  process.exit(1);
}

main();
