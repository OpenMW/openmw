#!/bin/bash

set -eou pipefail

commit="${CI_COMMIT_SHA:-$(git rev-parse HEAD)}"

access_token="${GITLAB_ACCESS_TOKEN:-${CI_JOB_TOKEN}}"
api="${CI_API_V4_URL:-https://gitlab.com/api/v4}"
project="$(printf '%s' "${CI_PROJECT_PATH:-OpenMW/openmw}" | jq -sRr '@uri')"
project_url="$api/projects/$project"

found_good_commit=""
for i in {1..100}; do
    commit_status_json="$(curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/repository/commits/${commit}/statuses?all=true")"
    if [[ $commit_status_json == '{"message":"404 Commit Not Found"}' ]]; then
        commit="$(git rev-parse "${commit}~1")"
        continue
    fi
    pipeline_ids="$(echo "$commit_status_json" | jq '[.[].pipeline_id] | unique | .[]')"
    while IFS= read -r pipeline_id; do
        if [[ -n $pipeline_id ]] && curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/pipelines/$pipeline_id" | jq -e '.status == "success" and .source == "push"' > /dev/null; then
            found_good_commit="1"
            break
        fi
    done <<< "$pipeline_ids"
    if [[ -n $found_good_commit ]]; then
        break
    else
        commit="$(curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/repository/commits/${commit}" | jq -r '.parent_ids[0]')"
    fi
done

echo $commit
