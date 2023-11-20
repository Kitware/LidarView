#!/bin/bash

set -e

readonly resources_path="Application/Qt/ApplicationComponents/Resources"
readonly schema_name="interface_modes_config.schema.json"
readonly json_name="interface_modes_config.json"

json validate --schema-file="$resources_path/$schema_name" --document-file="$resources_path/$json_name"
