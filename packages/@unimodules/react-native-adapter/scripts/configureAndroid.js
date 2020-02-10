#!/usr/bin/env node

const path = require('path');
const fs = require('fs-extra');
const chalk = require('chalk');
const inquirer = require('inquirer');

const { findNativeProjectAsync } = require('./tools');

const OLD_IMPORT_REGEXP = /apply\s+from:\s+['"][^'"]+\/react-native-unimodules\/gradle\.groovy['"]/g;
const NEW_IMPORT_REGEXP = /apply\s+from:\s+['"][^'"]+\/react-native-adapter\/autolinking\.gradle['"]/g;
const OLD_METHOD_REGEXP = /includeUnimodulesProjects/g;

const AUTOLINKING_GRADLE_PATH = path.join(__dirname, 'autolinking.gradle'); // eslint-disable-line

async function configureAndroid({ rootDir, nativeProjectDir, settingsGradleContent }) {
  console.log(chalk.yellow(settingsGradleContent));

  const transformedSettingsGradle = await transformSettingsGradle(
    settingsGradleContent,
    nativeProjectDir,
    rootDir
  );

  if (transformedSettingsGradle) {
    console.log(chalk.green(transformedSettingsGradle));
    // const pathToImport = settingsGradlePath.

    // await fs.writeFileAsync(settingsGradlePath, settingsGradleContents);
  }
}

function canMigrateAndroid(settingsGradleContent) {
  return (
    OLD_IMPORT_REGEXP.test(settingsGradleContent) || OLD_METHOD_REGEXP.test(settingsGradleContent)
  );
}

// Migrate from v1 provided by `react-native-unimodules`.
async function migrateAndroid(nativeProjectDir, settingsGradleContent) {
  console.log(
    chalk.cyan(
      `\nDetected an old version of autolinking - it's been moved from ${chalk.green(
        'react-native-unimodules'
      )} to ${chalk.green(
        '@unimodules/react-native-adapter'
      )} package and made much easier to setup.\nNow the configuration takes place only in ${chalk.yellow(
        'settings.gradle'
      )} file.\n`
    )
  );

  const { confirm } = await inquirer.prompt([
    {
      type: 'confirm',
      name: 'confirm',
      message: 'We can migrate it the new version for you. Do you want to proceed?',
    },
  ]);

  if (confirm) {
    const migratedContent = settingsGradleContent
      .replace(OLD_IMPORT_REGEXP, getNewStyleImportString(nativeProjectDir))
      .replace(/includeUnimodulesProjects/g, 'unimodules.includeProjects');

    console.log(chalk.green(migratedContent));
  } else {
    console.log(chalk.red("Migrating Android's autolinking has been declined. 🛑"));
  }
}

function getNewStyleImportString(nativeProjectDir) {
  return `apply from: '${path.relative(nativeProjectDir, AUTOLINKING_GRADLE_PATH)}'`;
}

async function transformSettingsGradle(originalContent, nativeProjectDir, rootDir) {
  const includeMethodString = 'unimodules.includeProjects';

  if (!NEW_IMPORT_REGEXP.test(originalContent) && !originalContent.includes(includeMethodString)) {
    const stringToAddArray = [
      '// Apply unimodules autolinking script and include all unimodules as local projects.',
      getNewStyleImportString(nativeProjectDir),
      `${includeMethodString}()`,
    ];

    console.log(
      chalk.cyan(
        `File ${chalk.yellow(
          'settings.gradle'
        )} doesn't seem to be configured, we can add these lines at the end:\n`
      )
    );

    // Print lines to add.
    console.log(chalk.gray(stringToAddArray[0]));
    stringToAddArray.slice(1).map(str => console.log(chalk.green(str)));
    console.log();

    const { confirm } = await inquirer.prompt([
      {
        type: 'confirm',
        name: 'confirm',
        message: `Do you want us to add these lines to ${chalk.yellow('settings.gradle')}?`,
      },
    ]);

    if (confirm) {
      return `${originalContent}\n\n${stringToAddArray.join('\n')}\n`;
    }
    console.log(chalk.red('Configuring Android project has been declined. 🛑'));
  } else {
    console.log(
      chalk.cyan(
        `Android project under ${chalk.yellow(
          path.relative(rootDir, nativeProjectDir)
        )} is already configured. ✅`
      )
    );
  }
  return null;
}

module.exports = async function(rootDir) {
  const nativeProjectDir = await findNativeProjectAsync(rootDir, '**/settings.gradle', 'Android');

  if (nativeProjectDir) {
    const settingsGradlePath = path.join(nativeProjectDir, 'settings.gradle');
    const settingsGradleContent = await fs.readFile(settingsGradlePath, 'utf8');

    if (canMigrateAndroid(settingsGradleContent)) {
      await migrateAndroid(nativeProjectDir, settingsGradleContent);
    } else {
      await configureAndroid({ rootDir, nativeProjectDir, settingsGradleContent });
    }
  } else {
    console.error(
      chalk.red(
        `No Android project found. If that's wrong, please make sure you have ${chalk.yellow(
          'settings.gradle'
        )} file in the root Android project folder.`
      )
    );
  }
};
