#!/usr/bin/env python3
"""
Script to bootstrap the `ldh` project for development.

Usage:
    bootstrap.py [-hcs] [-o dep]

Options:
  -c, --clean  Reset project to its default state.
  -s, --skip-pre  Skip all pre-install and post-remove steps.
  -o, --only dep  Run only for the specified dependency
  -h, --help  Print this help dialog.
"""
import enum
import logging
from logging.config import dictConfig
import os
import sys
from typing import Dict, List, Union

from docopt import docopt
import yaml
import pygit2


######################################################
# Set up logging
######################################################


LOGGING = {
    'version': 1,
    'disable_existing_loggers': True,
    'formatters': {
        'default': {
            'format': (
                '%(asctime)s %(levelname)s '
                '%(message)s'
            ),
        },
    },
    'handlers': {
        'console': {
            'class': 'logging.StreamHandler',
            'formatter': 'default',
            'stream': 'ext://sys.stdout',
        },
    },
    'loggers': {
        __name__: {
            'handlers': ['console'],
            'level': 'INFO',
        },
    },
}
dictConfig(LOGGING)

logger = logging.getLogger(__name__)


######################################################
# Data definitions
######################################################


class DependencyActionType(enum.Enum):
    LINK_FILE = 'link-file'
    LINK_INCLUDE_DIR = 'link-include-dir'


class PyGitCallbacks(pygit2.RemoteCallbacks):
    def credentials(self, url, username_from_url, allowed_types):
        if allowed_types & pygit2.credentials.GIT_CREDENTIAL_USERNAME:
            return pygit2.Username("git")
        elif allowed_types & pygit2.credentials.GIT_CREDENTIAL_SSH_KEY:
            return pygit2.Keypair("git", "id_rsa.pub", "id_rsa", "")
        else:
            return None


_RESET_FLAG = '--clean'
_SKIP_PRE_FLAG = '--skip-pre'
# TODO make this configurable
_CONFIGURATION_FILE_PATH = './submodules.yml'

_GIT_SM_DEINIT_FORMAT_STR = 'git submodule deinit {}'

_SUBMODULE_DIR = 'external-libs'


######################################################
# Main logic
######################################################


def execute_shell_commands(commands: List[str], wd: str = None) -> bool:
    """N.b. this was not written with safety in mind! Do not pass arguments that
    have been auto-generated and/or have not been manually inspected!
    """
    if wd:
        old_wd = os.getcwd()
        os.chdir(wd)

    try:
        for command in commands:
            # HACK check if command is `cd` and update wd accordingly
            split_command = command.split(' ')
            if split_command[0] == 'cd':
                os.chdir(os.path.join(os.getcwd(), split_command[1]))
            else:
                if os.system(command):
                    logger.error(f'Command "{command}" failed, aborting')
                    return False

        return True
    finally:
        if wd:
            os.chdir(old_wd)


def parse_config(config_path: str) -> Dict[str, Union[str, Dict, List[str]]]:
    with open(_CONFIGURATION_FILE_PATH, 'r') as f:
        try:
            submodules = yaml.safe_load(f)
        except yaml.YAMLError as exc:
            logger.error(exc)
            sys.exit(1)

    return submodules


def reset_project(skip_pre: bool = False, only_dependency: str = None):
    """This method should be the compliment of `bootstrap_project`, meaning that it should:
        - unlink all linked dependencies listed in the configuration file
        - fetches and updates all git submodules (equivalent of manually running
          `git submodule update --recursive` on the top level)
        - anything else you'd like :)
    """
    if input('Proceed with project cleanup? (y/[n]) > ') != 'y':
        logger.info('Aborting project cleanup')
        return

    submodules = parse_config(_CONFIGURATION_FILE_PATH)

    for submodule in submodules:
        submodule_name = submodule.get('name')
        if only_dependency and only_dependency != submodule_name:
            continue

        logger.info(f'Processing "{submodule_name}"')

        cwd = os.getcwd()

        submodule_preamble = submodule.get('pre-remove')
        if submodule_preamble and not skip_pre:
            preamble_result = execute_shell_commands(
                submodule_preamble, wd=os.path.join(cwd, _SUBMODULE_DIR, submodule_name),
            )
            if not preamble_result:
                logger.error(f'Removal preamble for dependency {submodule_name} failed, skipping')
                continue

        submodule_actions = submodule.get('actions')
        if not submodule_actions:
            logger.info(f'No action specified for "{submodule_name}", skipping.')
            continue

        for submodule_action in submodule_actions:
            try:
                submodule_action_type = DependencyActionType(submodule_action.get('type'))
            except ValueError:
                logger.warning(
                    f'Could not parse {submodule_action.get("type")}, valid options are'
                    f' {[t.value for t in DependencyActionType]}'
                )
                continue

            relative_dependency_dst = os.path.join(submodule_action.get('dst'))
            absolute_dependency_dst = os.path.join(cwd, relative_dependency_dst)

            if not os.path.islink(absolute_dependency_dst):
                logger.warning(f'Path "{relative_dependency_dst}" is not a link, will leave untouched')
                continue

            logger.info(f'Proceeding to unlink "{relative_dependency_dst}"')
            os.unlink(absolute_dependency_dst)


def bootstrap_project(skip_pre: bool = False, only_dependency: str = None):
    """This method takes care of the following:
        - fetches and updates all git submodules (equivalent of manually running
          `git submodule update --recursive` on the top level)
        - sets up links to libraries/included headers from our dependencies
        - anything else you'd like :)
    """
    if input('Proceed with project bootstrapping? (y/[n]) > ') != 'y':
        logger.info('Aborting project boostrapping')
        return

    os.makedirs('libs', exist_ok=True)

    submodules = parse_config(_CONFIGURATION_FILE_PATH)

    repo = pygit2.Repository('.')

    # NOTE for now, we assume that the specified submodules have been added externally, and
    # we're only concerned with fetching them.
    submodule_names = [s.get('name') for s in submodules]
    repo.init_submodules(submodules=submodule_names, overwrite=False)
    repo.update_submodules(
        submodules=[s.get('name') for s in submodules],
        init=False,
        callbacks=PyGitCallbacks(),
    )

    for submodule in submodules:
        submodule_name = submodule.get('name')
        if only_dependency and only_dependency != submodule_name:
            continue

        logger.info(f'Processing "{submodule_name}"')

        cwd = os.getcwd()

        submodule_preamble = submodule.get('pre-install')
        if submodule_preamble and not skip_pre:
            preamble_result = execute_shell_commands(
                submodule_preamble, wd=os.path.join(cwd, _SUBMODULE_DIR, submodule_name)
            )
            if not preamble_result:
                logger.error(f'Preamble for dependency {submodule_name} failed, skipping')
                continue

        submodule_actions = submodule.get('actions')
        if not submodule_actions:
            logger.info(f'No action specified for "{submodule_name}", skipping.')
            continue

        for submodule_action in submodule_actions:
            try:
                submodule_action_type = DependencyActionType(submodule_action.get('type'))
            except ValueError:
                logger.warning(
                    f'Could not parse {submodule_action.get("type")}, valid options are'
                    f' {[t.value for t in DependencyActionType]}'
                )
                break

            relative_dependency_src = os.path.join(_SUBMODULE_DIR, submodule.get('name'), submodule_action.get('src'))
            absolute_dependency_src = os.path.join(cwd, relative_dependency_src)
            relative_dependency_dst = os.path.join(submodule_action.get('dst'))
            absolute_dependency_dst = os.path.join(cwd, relative_dependency_dst)

            if os.path.islink(absolute_dependency_dst):
                logger.warning(
                    f'Path "{relative_dependency_dst}" is already a link, will leave untouched'
                )
                continue

            if submodule_action_type == DependencyActionType.LINK_FILE:
                logger.info(
                    f'Setting up link between source file "{absolute_dependency_src}" and destination'
                    f'file "{absolute_dependency_dst}"'
                )

                if not os.path.isfile(absolute_dependency_src):
                    logger.warning(
                        f'Path "{relative_dependency_src}" does not exist, ignoring.'
                    )
                    continue

                os.symlink(absolute_dependency_src, absolute_dependency_dst, target_is_directory=False)
            elif submodule_action_type == DependencyActionType.LINK_INCLUDE_DIR:
                logger.info(
                    f'Setting up link between source directory "{absolute_dependency_src}" and'
                    f' destination directory "{absolute_dependency_dst}"'
                )

                os.symlink(absolute_dependency_src, absolute_dependency_dst, target_is_directory=True)


def main(arguments):
    if arguments.get(_RESET_FLAG, False):
        reset_project(skip_pre=arguments.get(_SKIP_PRE_FLAG), only_dependency=arguments.get('--only'))
        return

    bootstrap_project(skip_pre=arguments.get(_SKIP_PRE_FLAG), only_dependency=arguments.get('--only'))

if __name__ == '__main__':
    arguments = docopt(__doc__)
    main(arguments)
