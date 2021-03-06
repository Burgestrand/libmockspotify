#include "libmockspotify.h"

sp_session *
sp_mock_session_create(const sp_session_config *config, sp_connectionstate connectionstate,
                      int offline_time_left, sp_offline_sync_status *sync_status,
                      int offline_num_playlists, int offline_tracks_to_sync, sp_playlist *inbox)
{
  sp_session *session = ALLOC(sp_session);

  sp_session_config *cloned_config = ALLOC(sp_session_config);
  if (config)
  {
    config = MEMCPY(cloned_config, config, sp_session_config);
  }
  cloned_config->application_key = "appkey_good";

  sp_session_create(cloned_config, &session);

  session->connectionstate = connectionstate;

  session->offline_time_left = offline_time_left;

  if (sync_status)
  {
    session->offline_sync_status = ALLOC(sp_offline_sync_status);
    MEMCPY(session->offline_sync_status, sync_status, sp_offline_sync_status);
  }

  session->offline_num_playlists = offline_num_playlists;
  session->offline_tracks_to_sync = offline_tracks_to_sync;

  session->inbox = inbox;

  return session;
}

void
sp_mock_session_set_is_scrobbling_possible(sp_session *session, sp_social_provider provider, bool possible)
{
  session->scrobbling[provider].is_possible = possible;
}

sp_error
sp_session_is_scrobbling_possible(sp_session *session, sp_social_provider provider, bool *out)
{
  *out = session->scrobbling[provider].is_possible;

  switch(provider)
  {
    case SP_SOCIAL_PROVIDER_FACEBOOK:
      return SP_ERROR_OK;

    // libspotify v12.1.56 has a bug where all providers but facebook return error
    default:
      return SP_ERROR_INVALID_INDATA;
  }
}

sp_error
sp_session_set_scrobbling(sp_session *session, sp_social_provider provider, sp_scrobbling_state state)
{
  if (state == SP_SCROBBLING_STATE_USE_GLOBAL_SETTING)
  {
    state = SP_SCROBBLING_STATE_GLOBAL_ENABLED;
  }

  session->scrobbling[provider].state = state;
  return SP_ERROR_OK;
}

sp_error
sp_session_is_scrobbling(sp_session *session, sp_social_provider provider, sp_scrobbling_state *out)
{
  *out = session->scrobbling[provider].state;
  return SP_ERROR_OK;
}

sp_error
sp_session_set_social_credentials(sp_session *UNUSED(session), sp_social_provider UNUSED(provider), const char *UNUSED(username), const char *UNUSED(password))
{
  return SP_ERROR_OK;
}

DEFINE_READER(session, connectionstate, sp_connectionstate);
DEFINE_READER(session, userdata, void *);
DEFINE_READER(session, is_private, bool);
DEFINE_READER(session, user, sp_user *);

const char * sp_build_id(void)
{
  return "12.1.51 (libmockspotify)";
}

sp_playlistcontainer *
sp_session_playlistcontainer(sp_session *session)
{
  return sp_session_publishedcontainer_for_user_create(session, NULL);
}

sp_playlistcontainer *
sp_session_publishedcontainer_for_user_create(sp_session *session, const char *username)
{
  char *link;

  if (sp_session_connectionstate(session) != SP_CONNECTION_STATE_LOGGED_IN)
  {
    return NULL;
  }

  if (username == NULL && session->user)
  {
    username = session->user->canonical_name;
  }

  link = ALLOC_STR(strlen("spotify:container:") + strlen(username));
  sprintf(link, "spotify:container:%s", username);
  return (sp_playlistcontainer *)sp_mock_registry_find(link);
}

sp_error
sp_session_create(const sp_session_config *config, sp_session * *sess)
{
  sp_session *session;

  if (memcmp(config->application_key, "appkey_good", config->application_key_size))
      return SP_ERROR_BAD_APPLICATION_KEY;

  session = *sess = ALLOC(sp_session);

  session->config.api_version       = config->api_version;
  session->config.cache_location    = strclone(config->cache_location);
  session->config.settings_location = strclone(config->settings_location);
  session->config.user_agent        = strclone(config->user_agent);
  session->config.callbacks         = ALLOC(sp_session_callbacks);
  session->config.userdata          = config->userdata;

  session->config.application_key   = ALLOC_N(byte, config->application_key_size);
  memcpy((char *) session->config.application_key, config->application_key, config->application_key_size);

  if (config->callbacks)
  {
    MEMCPY((sp_session_callbacks *) session->config.callbacks, config->callbacks, sp_session_callbacks);
  }

  // sp_session defaults
  session->cache_size = 0;

  // TODO: v0.0.8 (and earlier) directly call `notify_main_thread` callback here, before returning
  if (config->callbacks && config->callbacks->notify_main_thread)
    config->callbacks->notify_main_thread(NULL);

  return SP_ERROR_OK;
}

sp_error
sp_session_release(sp_session *UNUSED(session))
{
  return SP_ERROR_OK;
}

sp_error
sp_session_process_events(sp_session *UNUSED(session), int *next_timeout)
{
  *next_timeout = 1;
  return SP_ERROR_OK;
}

sp_error
sp_session_login(sp_session *session, const char *username, const char *UNUSED(password), bool remember_me, const char *UNUSED(blob))
{
  session->user = sp_mock_user_create(username, username, true);
  session->connectionstate = SP_CONNECTION_STATE_LOGGED_IN;

  if (remember_me)
  {
    session->remembered_user = strclone(username);
  }

  session->user_name = strclone(username);

  return SP_ERROR_OK;
}

sp_error
sp_session_relogin(sp_session *session)
{
  if ( ! session->remembered_user)
  {
    return SP_ERROR_NO_CREDENTIALS;
  }

  sp_session_login(session, session->remembered_user, NULL, true, NULL);
  return SP_ERROR_OK;
}

int
sp_session_remembered_user(sp_session *session, char *buffer, size_t buffer_size)
{
  if ( ! session->remembered_user)
  {
    return -1;
  }

  strncpy(buffer, session->remembered_user, buffer_size);

  if (buffer_size > 0)
  {
    buffer[buffer_size - 1] = '\0';
  }

  return (int) strlen(session->remembered_user);
}

const char *
sp_session_user_name(sp_session *session)
{
  if ( ! session->user_name)
  {
    return "";
  }

  return session->user_name;
}

sp_error
sp_session_forget_me(sp_session *session)
{
  session->remembered_user = NULL;
  return SP_ERROR_OK;
}

sp_error
sp_session_logout(sp_session *session)
{
  session->connectionstate = SP_CONNECTION_STATE_LOGGED_OUT;
  return SP_ERROR_OK;
}

sp_error
sp_session_flush_caches(sp_session *session)
{
  // no op
  return SP_ERROR_OK;
}

int
sp_session_user_country(sp_session *UNUSED(session))
{
  return ('S' << 8 | 'E');
}

sp_error
sp_session_set_cache_size(sp_session *session, size_t size)
{
  session->cache_size = size;
  return SP_ERROR_OK;
}

sp_error
sp_session_preferred_bitrate(sp_session *session, sp_bitrate bitrate)
{
  session->preferred_bitrate = bitrate;
  return SP_ERROR_OK;
}

sp_error
sp_session_set_connection_type(sp_session *session, sp_connection_type connection_type)
{
  session->connection_type = connection_type;
  return SP_ERROR_OK;
}

sp_error
sp_session_set_connection_rules(sp_session *session, sp_connection_rules connection_rules)
{
  session->connection_rules = connection_rules;
  return SP_ERROR_OK;
}

sp_error
sp_session_preferred_offline_bitrate(sp_session *UNUSED(session), sp_bitrate UNUSED(bitrate), bool UNUSED(allow_resync))
{
  return SP_ERROR_OK;
}

bool
sp_offline_sync_get_status(sp_session *session, sp_offline_sync_status *status)
{
  if (session->offline_sync_status)
  {
    MEMCPY(status, session->offline_sync_status, sp_offline_sync_status);
    return true;
  }

  return false;
}

int sp_offline_time_left(sp_session *x) { return x->offline_time_left; }
int sp_offline_num_playlists(sp_session *x) { return x->offline_num_playlists; }
int sp_offline_tracks_to_sync(sp_session *x) { return x->offline_tracks_to_sync; }

sp_playlist *
sp_session_starred_create(sp_session *session)
{
  if (sp_session_connectionstate(session) != SP_CONNECTION_STATE_LOGGED_IN)
  {
    return NULL;
  }

  return sp_session_starred_for_user_create(session, sp_user_canonical_name(session->user));
}

sp_playlist *
sp_session_starred_for_user_create(sp_session *session, const char *name)
{
  char *link;

  if (sp_session_connectionstate(session) != SP_CONNECTION_STATE_LOGGED_IN)
  {
    return NULL;
  }

  link = ALLOC_STR(strlen("spotify:user:") + strlen(name) + strlen(":starred"));
  sprintf(link, "spotify:user:%s:starred", name);
  return (sp_playlist *)sp_mock_registry_find(link);
}

sp_playlist *
sp_session_inbox_create(sp_session *session)
{
  if (sp_session_connectionstate(session) != SP_CONNECTION_STATE_LOGGED_IN)
  {
    return NULL;
  }

  return session->inbox;
}

bool
sp_session_get_volume_normalization(sp_session *session)
{
  return session->volume_normalization;
}

sp_error
sp_session_set_volume_normalization(sp_session *session, bool yepnope)
{
  session->volume_normalization = yepnope;
  return SP_ERROR_OK;
}

sp_error
sp_session_set_private_session(sp_session *session, bool is_private)
{
  session->is_private = is_private;
  return SP_ERROR_OK;
}

bool
sp_session_is_private_session(sp_session *session)
{
  return session->is_private;
}
