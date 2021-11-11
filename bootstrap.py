#!/usr/bin/env python3
"""
Script to bootstrap the `ldh` project for development.

Usage:
    bootstrap.py [-hc]

Options:
  -c, --clean  Reset project to its default state.
  -h, --help  Print this help dialog.
"""
import enum
import logging
from logging.config import dictConfig
import os
import sys

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
    LINK_HEADER = 'link-header'
    LINK_INCLUDE_DIR = 'link-include-dir'


_RESET_FLAG = '--clean'
# TODO make this configurable
_CONFIGURATION_FILE_PATH = './submodules.yml'

_GIT_SM_DEINIT_FORMAT_STR = 'git submodule deinit {}'


######################################################
# Main logic
######################################################


def parse_config(config_path):
    with open(_CONFIGURATION_FILE_PATH, 'r') as f:
        try:
            submodules = yaml.safe_load(f)
        except yaml.YAMLError as exc:
            logger.error(exc)
            sys.exit(1)

    return submodules


def reset_project():
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
        logger.info(f'Processing "{submodule_name}"')

        submodule_action = submodule.get('action')
        if not submodule_action:
            logger.info(f'No action specified for "{submodule_name}", skipping.')
            continue

        try:
            submodule_action_type = DependencyActionType(submodule_action.get('type'))
        except ValueError:
            logger.warn(
                f'Could not parse {submodule_action.get("type")}, valid options are'
                f' {[t.value for t in DependencyActionType]}'
            )
            continue

        cwd = os.getcwd()
        relative_dependency_dst = os.path.join(submodule_action.get('dst'))
        absolute_dependency_dst = os.path.join(cwd, relative_dependency_dst)

        if not os.path.islink(absolute_dependency_dst):
            logger.warn(f'Path "{relative_dependency_dst}" is not a link, will leave untouched')
            continue

        logger.info(f'Proceeding to unlink "{relative_dependency_dst}"')
        os.unlink(absolute_dependency_dst)


def bootstrap_project():
    """This method takes care of the following:
        - fetches and updates all git submodules (equivalent of manually running
          `git submodule update --recursive` on the top level)
        - sets up links to libraries/included headers from our dependencies
        - anything else you'd like :)
    """
    if input('Proceed with project bootstrapping? (y/[n]) > ') != 'y':
        logger.info('Aborting project boostrapping')
        return

    submodules = parse_config(_CONFIGURATION_FILE_PATH)

    # TODO FIXME enable
    """
    repo = pygit2.Repository('.')

    # NOTE for now, we assume that the specified submodules have been added externally, and
    # we're only concerned with fetching them.
    repo.update_submodules(
        submodules=[s.get('name') for s in submosules], init=True,
    )
    """

    for submodule in submodules:
        submodule_name = submodule.get('name')
        logger.info(f'Processing "{submodule_name}"')

        submodule_action = submodule.get('action')
        if not submodule_action:
            logger.info(f'No action specified for "{submodule_name}", skipping.')
            continue

        try:
            submodule_action_type = DependencyActionType(submodule_action.get('type'))
        except ValueError:
            logger.warn(
                f'Could not parse {submodule_action.get("type")}, valid options are'
                f' {[t.value for t in DependencyActionType]}'
            )
            continue

        cwd = os.getcwd()
        relative_dependency_src = os.path.join(submodule.get('name'), submodule_action.get('src'))
        absolute_dependency_src = os.path.join(cwd, relative_dependency_src)
        relative_dependency_dst = os.path.join(submodule_action.get('dst'))
        absolute_dependency_dst = os.path.join(cwd, relative_dependency_dst)

        if os.path.islink(absolute_dependency_dst):
            logger.warn(f'Path "{relative_dependency_dst}" is already a link, will leave untouched')
            continue

        if submodule_action_type == DependencyActionType.LINK_HEADER:
            logger.info(
                f'Setting up link between source "{absolute_dependency_src}" and destination'
                f' "{absolute_dependency_dst}"'
            )

            os.symlink(absolute_dependency_src, absolute_dependency_dst, target_is_directory=False)
        elif submodule_action_type == DependencyActionType.LINK_INCLUDE_DIR:
            logger.info(
                f'Setting up link between source directory "{absolute_dependency_src}" and'
                f' destination directory "{absolute_dependency_dst}"'
            )

            os.symlink(absolute_dependency_src, absolute_dependency_dst, target_is_directory=True)


def main(arguments):
    if arguments.get(_RESET_FLAG, False):
        reset_project()
        return

    bootstrap_project()

if __name__ == '__main__':
    arguments = docopt(__doc__)
    main(arguments)
