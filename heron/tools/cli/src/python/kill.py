# Copyright 2016 Twitter. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
''' kill.py '''
import heron.tools.cli.src.python.args as cli_args
import heron.tools.cli.src.python.cli_helper as cli_helper


def create_parser(subparsers):
  '''
  :param subparsers:
  :return:
  '''
  parser = cli_helper.create_parser(subparsers, 'kill', 'Kill a topology')
  cli_args.add_clean_stateful_checkpoints(parser)
  return parser


# pylint: disable=unused-argument
def run(command, parser, cl_args, unknown_args):
  '''
  :param command:
  :param parser:
  :param cl_args:
  :param unknown_args:
  :return:
  '''
  extra_args = []
  if cl_args['clean_stateful_checkpoints']:
    extra_args.append('--clean_stateful_checkpoints')
  return cli_helper.run(command, cl_args, "kill topology", extra_args)
