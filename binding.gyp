{
  'targets': [
    {
      'target_name': 'GraphicsMagick',
      'sources': [ 'GraphicsMagick.cc' ],
      'library_dirs': [
        '/usr/local/lib',
        '/opt/local/lib',
      ],
      'libraries': [ '-lGraphicsMagick' ],
    }
  ]
}
