#!/usr/bin/env python3

import click
import discord_webhook
import multiprocessing
import os
import pathlib
import requests
import urllib.parse


@click.command()
@click.option('--token_path', type=str, default=pathlib.Path.home() / '.gitlab_token',
              help='Path to text file with Gitlab token.')
@click.option('--project_id', type=int, default=7107382,
              help='Gitlab project id.')
@click.option('--job_id', type=int, default=os.getenv('CI_JOB_ID'),
              help='Gitlab job id.')
@click.option('--host', type=str, default='gitlab.com',
              help='Gitlab host.')
@click.option('--workers', type=int, default=10,
              help='Number of parallel workers.')
@click.option('--target_branch', type=str, default='master',
              help='Merge request target branch.')
@click.option('--begin_page', type=int, default=1,
              help='Begin with given /merge_requests page.')
@click.option('--end_page', type=int, default=4,
              help='End before given /merge_requests page.')
@click.option('--per_page', type=int, default=100,
              help='Number of merge requests per page.')
@click.option('--ignored_mrs_path', type=str,
              help='Path to a list of ignored MRs.')
def main(token_path, project_id, job_id, host, workers, target_branch, begin_page, end_page, per_page, ignored_mrs_path):
    headers = make_headers(token_path)
    base_url = f'https://{host}/api/v4/projects/{project_id}/'
    discord_webhook_url = os.getenv('DISCORD_WEBHOOK_URL')
    ignored_mrs = frozenset(read_ignored_mrs(ignored_mrs_path))
    checked = 0
    filtered = 0
    missing = 0
    missing_mrs = list()
    for page in range(begin_page, end_page):
        merge_requests = parse_gitlab_response(requests.get(
            url=urllib.parse.urljoin(base_url, 'merge_requests'),
            headers=headers,
            params=dict(state='merged', per_page=per_page, page=page),
        ))
        if not merge_requests:
            break
        checked += len(merge_requests)
        merge_requests = [v for v in merge_requests if v['target_branch'] == target_branch]
        if not merge_requests:
            continue
        filtered += len(merge_requests)
        with multiprocessing.Pool(workers) as pool:
            missing_merge_requests = pool.map(FilterMissingMergeRequest(headers, base_url), merge_requests)
            for mr in missing_merge_requests:
                if mr is None:
                    continue
                if mr['reference'] in ignored_mrs or mr['reference'].strip('!') in ignored_mrs:
                    print(f"Ignored MR {mr['reference']} ({mr['id']}) is missing from branch {mr['target_branch']},"
                          f" previously was merged as {mr['merge_commit_sha']}")
                    continue
                missing += 1
                missing_mrs.append(mr)
                print(f"MR {mr['reference']} ({mr['id']}) is missing from branch {mr['target_branch']},"
                      f" previously was merged as {mr['merge_commit_sha']}")
    print(f'Checked {checked} MRs ({filtered} with {target_branch} target branch), {missing} are missing')
    if discord_webhook_url is not None and missing_mrs:
        project_web_url = parse_gitlab_response(requests.get(url=base_url, headers=headers))['web_url'] + '/'
        discord_message = format_discord_message(missing=missing, filtered=filtered, target_branch=target_branch,
                                                 project_web_url=project_web_url, missing_mrs=missing_mrs,
                                                 job_id=job_id)
        print('Sending Discord notification...')
        print(discord_message)
        discord_webhook.DiscordWebhook(url=discord_webhook_url, content=discord_message, rate_limit_retry=True).execute()
    if missing_mrs:
        exit(-1)


def format_discord_message(missing, filtered, target_branch, project_web_url, missing_mrs, job_id):
    target_branch = format_link(target_branch, urllib.parse.urljoin(project_web_url, f'-/tree/{target_branch}'))
    job = f' by job ' + format_link(job_id, urllib.parse.urljoin(project_web_url, f'-/jobs/{job_id}')) if job_id else ''
    return (
        f'Found {missing} missing MRs out of {filtered} from {target_branch} target branch{job}:\n'
        + '\n'.join(format_missing_mr_message(v, project_web_url) for v in missing_mrs)
    )


def format_missing_mr_message(mr, project_web_url):
    web_url = mr.get('web_url')
    reference = mr['reference']
    target_branch = mr['target_branch']
    commit = mr['merge_commit_sha']
    if web_url is not None:
        reference = format_link(reference, web_url)
        target_branch = format_link(target_branch, urllib.parse.urljoin(project_web_url, f'-/tree/{target_branch}'))
        commit = format_link(commit, urllib.parse.urljoin(project_web_url, f'-/commit/{commit}'))
    return f"MR {reference} is missing from branch {target_branch}, previously was merged as {commit}"


def format_link(name, url):
    return f'[{name}]({url})'


class FilterMissingMergeRequest:
    def __init__(self, headers, base_url):
        self.headers = headers
        self.base_url = base_url

    def __call__(self, merge_request):
        commit_refs = requests.get(
            url=urllib.parse.urljoin(self.base_url, f"repository/commits/{merge_request['merge_commit_sha']}/refs"),
            headers=self.headers,
        ).json()
        if 'message' in commit_refs and commit_refs['message'] == '404 Commit Not Found':
            return merge_request
        if not present_in_branch(commit_refs, branch=merge_request['target_branch']):
            return merge_request


def present_in_branch(commit_refs, branch):
    return bool(next((v for v in commit_refs if v['type'] == 'branch' and v['name'] == branch), None))


def make_headers(token_path):
    job_token = os.environ.get('CI_JOB_TOKEN')
    if job_token is not None:
        print('Using auth token from CI_JOB_TOKEN env')
        return {'JOB_TOKEN': job_token}
    if not os.path.exists(token_path):
        print(f'Ignore absent token path: {token_path}')
        return dict()
    print(f'Using auth token from: {token_path}')
    return {'PRIVATE-TOKEN': read_token(token_path)}


def read_ignored_mrs(path):
    if path is None:
        return
    with open(path) as stream:
        for line in stream:
            yield line.strip()


def read_token(path):
    with open(path) as stream:
        return stream.readline().strip()


def parse_gitlab_response(response):
    response = response.json()
    if isinstance(response, dict):
        message = response.get('message')
        if message is not None:
            raise RuntimeError(f'Gitlab request has failed: {message}')
    return response


if __name__ == '__main__':
    main()
